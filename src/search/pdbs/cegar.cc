#include "cegar.h"

#include "abstract_solution_data.h"

#include "../option_parser.h"

#include "../tasks/projected_task.h"

#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"

#include "../utils/countdown_timer.h"
#include "../utils/math.h"

using namespace std;

namespace pdbs {
struct Flaw {
    int solution_index;
    int variable;

    Flaw(int solution_index, int variable)
            : solution_index(solution_index),
              variable(variable) {
    }
};

using FlawList = vector<Flaw>;

class Cegar {
    const int max_refinements;
    const int max_pdb_size;
    const int max_collection_size;
    const bool wildcard_plans;
    const AllowMerging allow_merging;
    const double max_time;
    const shared_ptr<AbstractTask> &task;
    const vector<FactPair> goals;
    unordered_set<int> blacklisted_variables;
    shared_ptr<utils::RandomNumberGenerator> rng;
    const utils::Verbosity verbosity;
    const string token = "CEGAR: ";

    // the pattern collection in form of their pdbs plus stored plans.
    vector<unique_ptr<AbstractSolutionData>> solutions;
    // Takes a variable as key and returns the index of the solutions-entry
    // whose pattern contains said variable. Used for checking if a variable
    // is already included in some pattern as well as for quickly finding
    // the other partner for merging.
    unordered_map<int, int> solution_lookup;
    int collection_size;

    // If the algorithm finds a single solution instance that solves
    // the concrete problem, then it will store its index here.
    // This enables simpler plan extraction later on.
    int concrete_solution_index;

    void print_collection() const;
    void compute_initial_collection();
    bool time_limit_reached(const utils::CountdownTimer &timer) const;
    bool termination_conditions_met(
            const utils::CountdownTimer &timer, int refinement_counter) const;

    /*
      Try to apply the specified abstract solution
      in concrete space by starting with the specified state.
      Return the last state that could be reached before the
      solution failed (if the solution did not fail, then the
      returned state is a goal state of the concrete task).
      The second element of the returned pair is a list of variables
      that caused the solution to fail.
     */
    FlawList apply_wildcard_plan(
            const shared_ptr<AbstractTask> &task, int solution_index, const State &init);
    FlawList get_flaws(const shared_ptr<AbstractTask> &task);

    // Methods related to refining (and adding patterns to the collection generally).
    void add_pattern_for_var(const shared_ptr<AbstractTask> &task, int var);
    bool can_merge_patterns(int index1, int index2) const;
    void merge_patterns(
            const shared_ptr<AbstractTask> &task, int index1, int index2);
    bool can_add_variable_to_pattern(
            const TaskProxy &task_proxy, int index, int var) const;
    void add_variable_to_pattern(
            const shared_ptr<AbstractTask> &task, int index, int var);
    void handle_flaw(
            const shared_ptr<AbstractTask> &task, const Flaw &flaw);
    void refine(const shared_ptr<AbstractTask> &task, const FlawList& flaws);
public:
    Cegar(
        int max_refinements,
        int max_pdb_size,
        int max_collection_size,
        bool wildcard_plans,
        AllowMerging allow_merging,
        double max_time,
        const shared_ptr<AbstractTask> &task,
        vector<FactPair> &&goals,
        unordered_set<int> &&blacklisted_variables,
        const shared_ptr<utils::RandomNumberGenerator> &rng,
        utils::Verbosity verbosity);

    PatternCollectionInformation run();
};

void Cegar::print_collection() const {
    utils::g_log << "[";
    for(size_t i = 0; i < solutions.size(); ++i) {
        const auto &sol = solutions[i];
        if (sol) {
            utils::g_log << sol->get_pattern();
            if (i != solutions.size() - 1) {
                utils::g_log << ", ";
            }
        }
    }
    utils::g_log << "]" << endl;
}

void Cegar::compute_initial_collection() {
    assert(!goals.empty());
    for (const FactPair &goal : goals) {
        add_pattern_for_var(task, goal.var);
    }

    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << token << "initial collection: ";
        print_collection();
        utils::g_log << endl;
    }
}

bool Cegar::time_limit_reached(
        const utils::CountdownTimer &timer) const {
    if (timer.is_expired()) {
        if (verbosity >= utils::Verbosity::NORMAL) {
            utils::g_log << token << "time limit reached" << endl;
        }
        return true;
    }
    return false;
}

bool Cegar::termination_conditions_met(
        const utils::CountdownTimer &timer, int refinement_counter) const {
    if (time_limit_reached(timer)) {
        return true;
    }

    if (refinement_counter == max_refinements) {
        if (verbosity >= utils::Verbosity::NORMAL) {
            utils::g_log << token << "maximum allowed number of refinements reached." << endl;
        }
        return true;
    }

    return false;
}

/*
  TODO: this is a duplicate of State::get_unregistered_successor.
  The reason is that we may apply operators even though they are not
  applicable due to ignoring blacklisted violated preconditions.
*/
State get_unregistered_successor(
    const shared_ptr<AbstractTask> &task,
    const State &state,
    const OperatorProxy &op) {
    assert(!op.is_axiom());
    vector<int> new_values = state.get_unpacked_values();

    for (EffectProxy effect : op.get_effects()) {
        if (does_fire(effect, state)) {
            FactPair effect_fact = effect.get_fact().get_pair();
            new_values[effect_fact.var] = effect_fact.value;
        }
    }

    assert(task->get_num_axioms() == 0);
    return State(*task, move(new_values));
}

FlawList Cegar::apply_wildcard_plan(
        const shared_ptr<AbstractTask> &task, int solution_index, const State &init) {
    TaskProxy task_proxy(*task);
    FlawList flaws;
    State current(init);
    current.unpack();
    AbstractSolutionData &solution = *solutions[solution_index];
    const vector<vector<OperatorID>> &plan = solution.get_plan();
    for (const vector<OperatorID> &equivalent_ops : plan) {
        bool step_failed = true;
        for (OperatorID abs_op_id : equivalent_ops) {
            // retrieve the concrete operator that corresponds to the abstracted one
            OperatorID op_id = solution.get_concrete_op_id_for_abs_op_id(abs_op_id, task);
            OperatorProxy op = task_proxy.get_operators()[op_id];

            // we do not use task_properties::is_applicable here because
            // checking for applicability manually allows us to directly
            // access the precondition that precludes the operator from
            // being applicable
            bool flaw_detected = false;
            for (FactProxy precondition : op.get_preconditions()) {
                int var = precondition.get_variable().get_id();

                // we ignore blacklisted variables
                if (blacklisted_variables.count(var)) {
                    continue;
                }

                if (current[precondition.get_variable()] != precondition) {
                    flaw_detected = true;
                    flaws.emplace_back(solution_index, var);
                }
            }

            // If there is an operator that is applicable, clear collected
            // flaws, apply it, and continue with the next plan step.
            if (!flaw_detected) {
                step_failed = false;
                flaws.clear();
                current = get_unregistered_successor(task, current, op);
                break;
            }
        }

        // If all equivalent operators cannot be applied, we have to stop
        // plan execution.
        if (step_failed) {
            break;
        }
    }

    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << token << "plan of pattern " << solution.get_pattern();
    }
    if (flaws.empty()) {
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << " successfully executed ";
        }
        if (task_properties::is_goal_state(task_proxy, current)) {
            /*
              If there are no flaws, this does not guarantee that the plan
              is valid in the concrete state space because we might have
              ignored variables that have been blacklisted. Hence the tests
              for empty blacklists.
            */
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << "and resulted in a concrete goal state." << endl;
            }
            if (blacklisted_variables.empty()) {
                if (verbosity >= utils::Verbosity::VERBOSE) {
                    utils::g_log << token << "since there are no blacklisted variables, "
                                    "the concrete task is solved." << endl;
                }
                concrete_solution_index = solution_index;
            } else {
                if (verbosity >= utils::Verbosity::VERBOSE) {
                    utils::g_log << token << "since there are blacklisted variables, the plan "
                                    "is not guaranteed to work in the concrete state "
                                    "space. Marking this solution as solved." << endl;
                }
                solution.mark_as_solved();
            }
        } else {
            /*
              The pattern is missing at least one goal variable. Since all
              goal variables are in the collection from the start on, this
              flaw can only be fixed by merging patterns. Hence we raise
              flaws depending on the option allow_merging.
            */
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << "but did not lead to a goal state." << endl;
            }
            if (allow_merging == AllowMerging::AllFlaws) {
                for (const FactPair &goal : goals) {
                    int goal_var_id = goal.var;
                    if (current[goal_var_id].get_pair() != goal && !blacklisted_variables.count(goal_var_id)) {
                        flaws.emplace_back(solution_index, goal_var_id);
                    }
                }
                if (flaws.empty()) {
                    utils::g_log << token
                                 << "no non-blacklisted goal variables left, "
                                    "marking this pattern as solved." << endl;
                    solution.mark_as_solved();
                } else {
                    if (verbosity >= utils::Verbosity::VERBOSE) {
                        utils::g_log << token << "raising goal violation flaw(s)." << endl;
                    }
                }
            } else {
                if (verbosity >= utils::Verbosity::VERBOSE) {
                    utils::g_log << "we do not allow merging due to goal violation flaws, ";
                    utils::g_log << "thus marking this pattern as solved." << endl;
                }
                solution.mark_as_solved();
            }
        }
    } else {
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << " failed." << endl;
        }
    }

    return flaws;
}

FlawList Cegar::get_flaws(
        const shared_ptr<AbstractTask> &task) {
    FlawList flaws;
    State concrete_init = TaskProxy(*task).get_initial_state();

    for (size_t solution_index = 0;
         solution_index < solutions.size(); ++solution_index) {
        if (!solutions[solution_index] ||
            solutions[solution_index]->is_solved()) {
            continue;
        }

        AbstractSolutionData &solution = *solutions[solution_index];

        // abort here if no abstract solution could be found
        if (!solution.solution_exists()) {
            utils::g_log << token << "Problem unsolvable" << endl;
            utils::exit_with(utils::ExitCode::SEARCH_UNSOLVABLE);
        }

        // find out if and why the abstract solution
        // would not work for the concrete task.
        // We always start with the initial state.
        FlawList new_flaws = apply_wildcard_plan(
                task, solution_index, concrete_init);

        if (concrete_solution_index != -1) {
            // We solved the concrete task. Return empty flaws to signal terminating.
            assert(concrete_solution_index == static_cast<int>(solution_index));
            assert(new_flaws.empty());
            assert(blacklisted_variables.empty());
            flaws.clear();
            return flaws;
        }
        for (Flaw &flaw : new_flaws) {
            flaws.push_back(move(flaw));
        }
    }
    return flaws;
}

void Cegar::add_pattern_for_var(
        const shared_ptr<AbstractTask> &task, int var) {
    solutions.emplace_back(
            new AbstractSolutionData(task, {var}, rng, wildcard_plans, verbosity));
    solution_lookup[var] = solutions.size() - 1;
    collection_size += solutions.back()->get_pdb()->get_size();
}

bool Cegar::can_merge_patterns(
        int index1, int index2) const {
    int pdb_size1 = solutions[index1]->get_pdb()->get_size();
    int pdb_size2 = solutions[index2]->get_pdb()->get_size();
    if (!utils::is_product_within_limit(pdb_size1, pdb_size2, max_pdb_size)) {
        return false;
    }
    int added_size = pdb_size1 * pdb_size2 - pdb_size1 - pdb_size2;
    return collection_size + added_size <= max_collection_size;
}

void Cegar::merge_patterns(
        const shared_ptr<AbstractTask> &task, int index1, int index2) {
    // Merge pattern at index2 into pattern at index2
    AbstractSolutionData &solution1 = *solutions[index1];
    AbstractSolutionData &solution2 = *solutions[index2];

    const Pattern& pattern2 = solution2.get_pattern();

    // update look-up table
    for (int var : pattern2) {
        solution_lookup[var] = index1;
    }

    // compute merged pattern
    Pattern new_pattern = solution1.get_pattern();
    new_pattern.insert(new_pattern.end(), pattern2.begin(), pattern2.end());
    sort(new_pattern.begin(), new_pattern.end());

    // store old pdb sizes
    int pdb_size1 = solutions[index1]->get_pdb()->get_size();
    int pdb_size2 = solutions[index2]->get_pdb()->get_size();

    // compute merge solution
    unique_ptr<AbstractSolutionData> merged =
            utils::make_unique_ptr<AbstractSolutionData>(
                    task, new_pattern, rng, wildcard_plans, verbosity);

    // update collection size
    collection_size -= pdb_size1;
    collection_size -= pdb_size2;
    collection_size += merged->get_pdb()->get_size();

    // clean-up
    solutions[index1] = move(merged);
    solutions[index2] = nullptr;
}

bool Cegar::can_add_variable_to_pattern(
        const TaskProxy &task_proxy, int index, int var) const {
    int pdb_size = solutions[index]->get_pdb()->get_size();
    int domain_size = task_proxy.get_variables()[var].get_domain_size();
    if (!utils::is_product_within_limit(pdb_size, domain_size, max_pdb_size)) {
        return false;
    }
    int added_size = pdb_size * domain_size - pdb_size;
    return collection_size + added_size <= max_collection_size;
}

void Cegar::add_variable_to_pattern(
        const shared_ptr<AbstractTask> &task, int index, int var) {
    AbstractSolutionData &solution = *solutions[index];

    // compute new pattern
    Pattern new_pattern(solution.get_pattern());
    new_pattern.push_back(var);
    sort(new_pattern.begin(), new_pattern.end());

    // compute new solution
    unique_ptr<AbstractSolutionData> new_solution =
            utils::make_unique_ptr<AbstractSolutionData>(
                    task, new_pattern, rng, wildcard_plans, verbosity);

    // update collection size
    collection_size -= solution.get_pdb()->get_size();
    collection_size += new_solution->get_pdb()->get_size();

    // update look-up table
    solution_lookup[var] = index;
    solutions[index] = move(new_solution);
}

void Cegar::handle_flaw(
        const shared_ptr<AbstractTask> &task, const Flaw &flaw) {
    int sol_index = flaw.solution_index;
    int var = flaw.variable;
    bool added_var = false;
    auto it = solution_lookup.find(var);
    if (it != solution_lookup.end()) {
        // var is already in another pattern of the collection
        int other_index = it->second;
        assert(other_index != sol_index);
        assert(solutions[other_index] != nullptr);
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << token << "var" << var << " is already in pattern "
                         << solutions[other_index]->get_pattern() << endl;
        }
        if (allow_merging >= AllowMerging::PreconditionFlaws && can_merge_patterns(sol_index, other_index)) {
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << token << "merge the two patterns" << endl;
            }
            merge_patterns(task, sol_index, other_index);
            added_var = true;
        }
    } else {
        // var is not yet in the collection
        // Note on precondition violations: var may be a goal variable but
        // nevertheless is added to the pattern causing the flaw and not to
        // a single new pattern.
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << token << "var" << var
                         << " is not in the collection yet" << endl;
        }
        TaskProxy task_proxy(*task);
        if (can_add_variable_to_pattern(task_proxy, sol_index, var)) {
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << token << "add it to the pattern" << endl;
            }
            add_variable_to_pattern(task, sol_index, var);
            added_var = true;
        }
    }

    if (!added_var) {
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << token << "Could not add var/merge patterns due to size "
                                     "limits. Blacklisting." << endl;
        }
        blacklisted_variables.insert(var);
    }
}

void Cegar::refine(
        const shared_ptr<AbstractTask> &task, const FlawList &flaws) {
    assert(!flaws.empty());

    // pick a random flaw
    int random_flaw_index = (*rng)(flaws.size());
    const Flaw &flaw = flaws[random_flaw_index];

    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << token << "chosen flaw: pattern "
                     << solutions[flaw.solution_index]->get_pattern()
                     << " with a flaw on " << flaw.variable << endl;
    }
    handle_flaw(task, flaw);
}

PatternCollectionInformation Cegar::run() {
    utils::CountdownTimer timer(max_time);
    compute_initial_collection();
    int refinement_counter = 0;
    while (!termination_conditions_met(timer, refinement_counter)) {
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "iteration #" << refinement_counter + 1 << endl;
        }

        // vector of solution indices and flaws associated with said solutions
        FlawList flaws = get_flaws(task);

        if (flaws.empty()) {
            if (verbosity >= utils::Verbosity::NORMAL) {
                if (concrete_solution_index != -1) {
                    utils::g_log << token
                                 << "task solved during computation of abstract solutions"
                                 << endl;
                } else {
                    utils::g_log << token
                                 << "Flaw list empty. No further refinements possible."
                                 << endl;
                }
            }
            break;
        }

        if (time_limit_reached(timer)) {
            break;
        }

        // if there was a flaw, then refine the abstraction
        // such that said flaw does not occur again
        refine(task, flaws);

        ++refinement_counter;
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << token << "current collection size: " << collection_size << endl;
            utils::g_log << token << "current collection: ";
            print_collection();
        }
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << endl;
        }
    }
    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << endl;
    }

    shared_ptr<PatternCollection> patterns = make_shared<PatternCollection>();
    shared_ptr<PDBCollection> pdbs = make_shared<PDBCollection>();
    if (concrete_solution_index != -1) {
        const shared_ptr<PatternDatabase> &pdb =
                solutions[concrete_solution_index]->get_pdb();
        pdbs->push_back(pdb);
        patterns->push_back(pdb->get_pattern());
    } else {
        for (const auto &sol : solutions) {
            if (sol) {
                const shared_ptr<PatternDatabase> &pdb = sol->get_pdb();
                pdbs->push_back(pdb);
                patterns->push_back(pdb->get_pattern());
            }
        }
    }

    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << token << "computation time: " << timer.get_elapsed_time() << endl;
        utils::g_log << token << "number of iterations: " << refinement_counter << endl;
        utils::g_log << token << "final collection: " << *patterns << endl;
        utils::g_log << token << "final collection number of patterns: " << patterns->size() << endl;
        utils::g_log << token << "final collection summed PDB sizes: " << collection_size << endl;
    }

    TaskProxy task_proxy(*task);
    PatternCollectionInformation pattern_collection_information(
        task_proxy, patterns);
    pattern_collection_information.set_pdbs(pdbs);
    return pattern_collection_information;
}

Cegar::Cegar(
    int max_refinements,
    int max_pdb_size,
    int max_collection_size,
    bool wildcard_plans,
    AllowMerging allow_merging,
    double max_time,
    const shared_ptr<AbstractTask> &task,
    vector<FactPair> &&goals,
    unordered_set<int> &&blacklisted_variables,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    utils::Verbosity verbosity)
    : max_refinements(max_refinements),
      max_pdb_size(max_pdb_size),
      max_collection_size(max_collection_size),
      wildcard_plans(wildcard_plans),
      allow_merging(allow_merging),
      max_time(max_time),
      task(task),
      goals(move(goals)),
      blacklisted_variables(move(blacklisted_variables)),
      rng(rng),
      verbosity(verbosity),
      collection_size(0),
      concrete_solution_index(-1) {
}

PatternCollectionInformation cegar(
    int max_refinements,
    int max_pdb_size,
    int max_collection_size,
    bool wildcard_plans,
    AllowMerging allow_merging,
    double max_time,
    const shared_ptr<AbstractTask> &task,
    vector<FactPair> &&goals,
    unordered_set<int> &&blacklisted_variables,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    utils::Verbosity verbosity) {
#ifndef NDEBUG
    TaskProxy task_proxy(*task);
    for (const FactPair goal : goals) {
        bool is_goal = false;
        for (const FactProxy &task_goal : task_proxy.get_goals()) {
            if (goal == task_goal.get_pair()) {
                is_goal = true;
                break;
            }
        }
        if (!is_goal) {
            cerr << " Given goal is not a goal of the task" << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
    }
#endif
    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << "Options of the CEGAR algorithm for computing a pattern collection: " << endl;
        utils::g_log << "max refinements: " << max_refinements << endl;
        utils::g_log << "max pdb size: " << max_pdb_size << endl;
        utils::g_log << "max collection size: " << max_collection_size << endl;
        utils::g_log << "wildcard plans: " << wildcard_plans << endl;
        utils::g_log << "allow merging: ";
        switch (allow_merging) {
            case AllowMerging::Never:
                utils::g_log << "never";
                break;
            case AllowMerging::PreconditionFlaws:
                utils::g_log << "normal";
                break;
            case AllowMerging::AllFlaws:
                utils::g_log << "verbose";
                break;
        }
        utils::g_log << endl;
        utils::g_log << "Verbosity: ";
        switch (verbosity) {
            case utils::Verbosity::SILENT:
                utils::g_log << "silent";
                break;
            case utils::Verbosity::NORMAL:
                utils::g_log << "normal";
                break;
            case utils::Verbosity::VERBOSE:
                utils::g_log << "verbose";
                break;
            case utils::Verbosity::DEBUG:
                utils::g_log << "debug";
                break;
        }
        utils::g_log << endl;
        utils::g_log << "max time: " << max_time << endl;
        utils::g_log << "blacklisted variables: ";
        if (blacklisted_variables.empty()) {
            utils::g_log << "none";
        } else {
            for (int var : blacklisted_variables) {
                utils::g_log << var << ", ";
            }
        }
        utils::g_log << endl;
    }

    Cegar cegar(
        max_refinements,
        max_pdb_size,
        max_collection_size,
        wildcard_plans,
        allow_merging,
        max_time,
        task,
        move(goals),
        move(blacklisted_variables),
        rng,
        verbosity);
    return cegar.run();
}

void add_cegar_options_to_parser(options::OptionParser &parser) {
    parser.add_option<int>(
        "max_refinements",
        "maximum allowed number of refinements",
        "infinity",
        Bounds("0", "infinity"));
    parser.add_option<int>(
        "max_pdb_size",
        "maximum allowed number of states in a pdb (not applied to initial "
        "goal variable pattern(s))",
        "1000000",
        Bounds("1", "infinity"));
    parser.add_option<int>(
        "max_collection_size",
        "limit for the total number of PDB entries across all PDBs (not "
        "applied to initial goal variable pattern(s))",
        "infinity",
        Bounds("1","infinity"));
    parser.add_option<bool>(
        "wildcard_plans",
        "Make the algorithm work with wildcard rather than regular plans.",
        "true");
    vector<string> allow_merging_options;
    vector<string> allow_merging_doc;
    allow_merging_options.push_back("never");
    allow_merging_doc.push_back("never merge patterns");
    allow_merging_options.push_back("precondition_flaws");
    allow_merging_doc.push_back("only allow merging after precondition flaws");
    allow_merging_options.push_back("all_flaws");
    allow_merging_doc.push_back("allow merging for both goal and precondition flaws");
    parser.add_enum_option<AllowMerging>(
        "allow_merging",
        allow_merging_options,
        "ignore goal violations and consequently generate a single pattern",
        "all_flaws",
        allow_merging_doc);
    parser.add_option<double>(
        "max_time",
        "maximum time in seconds for CEGAR pattern generation. "
        "This includes the creation of the initial PDB collection"
        " as well as the creation of the correlation matrix.",
        "infinity",
        Bounds("0.0", "infinity"));
}
}
