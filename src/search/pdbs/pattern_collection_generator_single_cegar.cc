#include "pattern_collection_generator_single_cegar.h"

#include "abstract_solution_data.h"
#include "pattern_database.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../tasks/projected_task.h"

#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"

#include "../utils/countdown_timer.h"
#include "../utils/logging.h"
#include "../utils/math.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"
#include "../utils/system.h"

using namespace std;

namespace pdbs {
PatternCollectionGeneratorSingleCegar::PatternCollectionGeneratorSingleCegar(
        const shared_ptr<utils::RandomNumberGenerator> &arg_rng,
        int arg_max_refinements,
        int arg_max_pdb_size,
        int arg_max_collection_size,
        bool arg_wildcard_plans,
        bool arg_ignore_goal_violations,
        int global_blacklist_size,
        InitialCollectionType arg_initial,
        int given_goal,
        utils::Verbosity verbosity,
        double arg_max_time)
        : rng(arg_rng),
          max_refinements(arg_max_refinements),
          max_pdb_size(arg_max_pdb_size),
          max_collection_size(arg_max_collection_size),
          wildcard_plans(arg_wildcard_plans),
          ignore_goal_violations(arg_ignore_goal_violations),
          global_blacklist_size(global_blacklist_size),
          initial(arg_initial),
          given_goal(given_goal),
          verbosity(verbosity),
          max_time(arg_max_time),
          collection_size(0),
          concrete_solution_index(-1) {

    if (initial == InitialCollectionType::GIVEN_GOAL && given_goal == -1) {
        cerr << "Initial collection type 'given goal', but no goal specified" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }

    if (initial != InitialCollectionType::GIVEN_GOAL && given_goal != -1) {
        cerr << "Goal given, but initial collection type is not set to use it" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }

    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << token << "options: " << endl;
        utils::g_log << token << "max refinements: " << max_refinements << endl;
        utils::g_log << token << "max pdb size: " << max_pdb_size << endl;
        utils::g_log << token << "max collection size: " << max_collection_size << endl;
        utils::g_log << token << "wildcard plans: " << wildcard_plans << endl;
        utils::g_log << token << "ignore goal violations: " << ignore_goal_violations << endl;
        utils::g_log << token << "global blacklist size: " << global_blacklist_size << endl;
        utils::g_log << token << "initial collection type: ";
        switch (initial) {
        case InitialCollectionType::GIVEN_GOAL:
            utils::g_log << "given goal" << endl;
            break;
        case InitialCollectionType::RANDOM_GOAL:
            utils::g_log << "random goal" << endl;
            break;
        case InitialCollectionType::ALL_GOALS:
            utils::g_log << "all goals" << endl;
            break;
        }
        utils::g_log << token << "given goal: " << given_goal << endl;
        utils::g_log << token << "Verbosity: ";
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
        utils::g_log << token << "max time: " << max_time << endl;
    }
    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << endl;
    }
}

PatternCollectionGeneratorSingleCegar::PatternCollectionGeneratorSingleCegar(
        const options::Options &opts)
        : PatternCollectionGeneratorSingleCegar(
        utils::parse_rng_from_options(opts),
        opts.get<int>("max_refinements"),
        opts.get<int>("max_pdb_size"),
        opts.get<int>("max_collection_size"),
        opts.get<bool>("wildcard_plans"),
        opts.get<bool>("ignore_goal_violations"),
        opts.get<int>("global_blacklist_size"),
        opts.get<InitialCollectionType>("initial"),
        opts.get<int>("given_goal"),
        opts.get<utils::Verbosity>("verbosity"),
        opts.get<double>("max_time")) {
}

PatternCollectionGeneratorSingleCegar::~PatternCollectionGeneratorSingleCegar() {
}

void PatternCollectionGeneratorSingleCegar::print_collection() const {
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

void PatternCollectionGeneratorSingleCegar::generate_trivial_solution_collection(
    const shared_ptr<AbstractTask> &task) {
    assert(!remaining_goals.empty());

    switch (initial) {
        case InitialCollectionType::GIVEN_GOAL: {
            assert(given_goal != -1);
            update_goals(given_goal);
            add_pattern_for_var(task, given_goal);
            break;
        }
        case InitialCollectionType::RANDOM_GOAL: {
            int var = remaining_goals.back();
            remaining_goals.pop_back();
            add_pattern_for_var(task, var);
            break;
        }
        case InitialCollectionType::ALL_GOALS: {
            while (!remaining_goals.empty()) {
                int var = remaining_goals.back();
                remaining_goals.pop_back();
                add_pattern_for_var(task, var);
            }
            break;
        }
    }

    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << token << "initial collection: ";
        print_collection();
        utils::g_log << endl;
    }
}

bool PatternCollectionGeneratorSingleCegar::time_limit_reached(
    const utils::CountdownTimer &timer) const {
    if (timer.is_expired()) {
        if (verbosity >= utils::Verbosity::NORMAL) {
            utils::g_log << token << "time limit reached" << endl;
        }
        return true;
    }
    return false;
}

bool PatternCollectionGeneratorSingleCegar::termination_conditions_met(
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

FlawList PatternCollectionGeneratorSingleCegar::apply_wildcard_plan(
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
                if (global_blacklist.count(var)) {
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
                current = current.get_unregistered_successor(op);
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
                utils::g_log << " and resulted in a concrete goal state: ";
            }
            if (global_blacklist.empty()) {
                if (verbosity >= utils::Verbosity::VERBOSE) {
                    utils::g_log << "since there are no blacklisted variables, "
                            "the concrete task is solved." << endl;
                }
                concrete_solution_index = solution_index;
            } else {
                if (verbosity >= utils::Verbosity::VERBOSE) {
                    utils::g_log << "since there are blacklisted variables, the plan "
                            "is not guaranteed to work in the concrete state "
                            "space. Marking this solution as solved." << endl;
                }
                solution.mark_as_solved();
            }
        } else {
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << "but did not lead to a goal state: ";
            }
            if (!ignore_goal_violations) {
                if (verbosity >= utils::Verbosity::VERBOSE) {
                    utils::g_log << "potentially raising goal violation flaw(s)" << endl;
                }
                // Collect all non-satisfied goal variables that are still available.
                for (FactProxy goal : task_proxy.get_goals()) {
                    VariableProxy goal_var = goal.get_variable();
                    int goal_var_id = goal_var.get_id();
                    if (current[goal_var] != goal && !global_blacklist.count(goal_var_id) &&
                        find(remaining_goals.begin(), remaining_goals.end(),
                             goal_var_id)
                            != remaining_goals.end()) {
                        flaws.emplace_back(solution_index, goal_var_id);
                    }
                }
            } else {
                if (verbosity >= utils::Verbosity::VERBOSE) {
                    if (ignore_goal_violations) {
                        utils::g_log << "we ignore goal violations";
                    } else {
                        utils::g_log << "no more goals that could be added to the collection";
                    }
                    utils::g_log << ", thus marking this pattern as solved." << endl;
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

FlawList PatternCollectionGeneratorSingleCegar::get_flaws(
    const shared_ptr<AbstractTask> &task) {
    FlawList flaws;
    State concrete_init = TaskProxy(*task).get_initial_state();

    for (size_t solution_index = 0; solution_index < solutions.size(); ++solution_index) {
        if (!solutions[solution_index] || solutions[solution_index]->is_solved()) {
            continue;
        }

        AbstractSolutionData& solution = *solutions[solution_index];

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
            flaws.clear();
            return flaws;
        }
        for (Flaw &flaw : new_flaws) {
            flaws.push_back(move(flaw));
        }
    }
    return flaws;
}

void PatternCollectionGeneratorSingleCegar::update_goals(int added_var) {
    /*
      Only call this method if added_var is definitely added to some
      pattern. It removes the variable from remaining_goals if it is
      contained there.
    */
    vector<int>::iterator result = find(
        remaining_goals.begin(), remaining_goals.end(), added_var);
    if (result != remaining_goals.end()) {
        remaining_goals.erase(result);
    }
}

void PatternCollectionGeneratorSingleCegar::add_pattern_for_var(
    const shared_ptr<AbstractTask> &task, int var) {
    solutions.emplace_back(
        new AbstractSolutionData(task, {var}, rng, wildcard_plans, verbosity));
    solution_lookup[var] = solutions.size() - 1;
    collection_size += solutions.back()->get_pdb()->get_size();
}

bool PatternCollectionGeneratorSingleCegar::can_merge_patterns(
        int index1, int index2) const {
    int pdb_size1 = solutions[index1]->get_pdb()->get_size();
    int pdb_size2 = solutions[index2]->get_pdb()->get_size();
    if (!utils::is_product_within_limit(pdb_size1, pdb_size2, max_pdb_size)) {
        return false;
    }
    int added_size = pdb_size1 * pdb_size2 - pdb_size1 - pdb_size2;
    return collection_size + added_size <= max_collection_size;
}

void PatternCollectionGeneratorSingleCegar::merge_patterns(
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
    solutions[index1] = std::move(merged);
    solutions[index2] = nullptr;
}

bool PatternCollectionGeneratorSingleCegar::can_add_variable_to_pattern(
    const TaskProxy &task_proxy, int index, int var) const {
    int pdb_size = solutions[index]->get_pdb()->get_size();
    int domain_size = task_proxy.get_variables()[var].get_domain_size();
    if (!utils::is_product_within_limit(pdb_size, domain_size, max_pdb_size)) {
        return false;
    }
    int added_size = pdb_size * domain_size - pdb_size;
    return collection_size + added_size <= max_collection_size;
}

void PatternCollectionGeneratorSingleCegar::add_variable_to_pattern(
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

    // update look-up table and possibly remaining_goals, clean-up
    solution_lookup[var] = index;
    update_goals(var);
    solutions[index] = move(new_solution);
}

void PatternCollectionGeneratorSingleCegar::handle_flaw(
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
        if (can_merge_patterns(sol_index, other_index)) {
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
        global_blacklist.insert(var);
    }
}

void PatternCollectionGeneratorSingleCegar::refine(
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


PatternCollectionInformation PatternCollectionGeneratorSingleCegar::generate(
        const std::shared_ptr<AbstractTask> &task) {
    utils::CountdownTimer timer(max_time);
    TaskProxy task_proxy(*task);

    if (given_goal != -1 &&
            given_goal >= static_cast<int>(task_proxy.get_variables().size())) {
        cerr << "Goal variable out of range of task's variables" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }

    // save all goals in random order for refinement later
    bool found_given_goal = false;
    for (auto goal : task_proxy.get_goals()) {
        int goal_var = goal.get_variable().get_id();
        remaining_goals.push_back(goal_var);
        if (given_goal != -1 && goal_var == given_goal) {
            found_given_goal = true;
        }
    }
    if (given_goal != -1 && !found_given_goal) {
        cerr << " Given goal variable is not a goal variable" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
    rng->shuffle(remaining_goals);

    if (global_blacklist_size) {
        int num_vars = task_proxy.get_variables().size();
        vector<int> nongoals;
        nongoals.reserve(num_vars - remaining_goals.size());
        for (int var_id = 0; var_id < num_vars; ++var_id) {
            if (find(remaining_goals.begin(), remaining_goals.end(), var_id)
                == remaining_goals.end()) {
                nongoals.push_back(var_id);
            }
        }
        rng->shuffle(nongoals);

        // Select a random subset of non goals.
        for (size_t i = 0;
             i < min(static_cast<size_t>(global_blacklist_size),nongoals.size());
             ++i) {
            int var_id = nongoals[i];
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << token << "blacklisting var" << var_id << endl;
            }
            global_blacklist.insert(var_id);
        }
    }

    // Start with a solution of the trivial abstraction
    generate_trivial_solution_collection(task);

    // main loop of the algorithm
    int refinement_counter = 0;
    while (not termination_conditions_met(timer, refinement_counter)) {
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "iteration #" << refinement_counter + 1 << endl;
        }

        // vector of solution indices and flaws associated with said solutions
        FlawList flaws = get_flaws(task);

        if (flaws.empty()) {
            if (concrete_solution_index != -1) {
                assert(global_blacklist.empty());
                if (verbosity >= utils::Verbosity::NORMAL) {
                    utils::g_log << token
                         << "task solved during computation of abstract solutions"
                         << endl;
                    solutions[concrete_solution_index]->print_plan();
                    utils::g_log << token << "length of plan: "
                         << solutions[concrete_solution_index]->get_plan().size()
                         << " step(s)." << endl;
                    utils::g_log << token << "cost of plan: "
                         << solutions[concrete_solution_index]->compute_plan_cost() << endl;
                }
            } else {
                if (verbosity >= utils::Verbosity::NORMAL) {
                    utils::g_log << token << "Flaw list empty. No further refinements possible." << endl;
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
    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << endl;
    }

    std::shared_ptr<PatternCollection> patterns = make_shared<PatternCollection>();
    std::shared_ptr<PDBCollection> pdbs = make_shared<PDBCollection>();
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
        utils::g_log << token << "final collection: " << *patterns << endl << endl;
        utils::g_log << token << "final collection number of patterns: " << patterns->size() << endl;
        utils::g_log << token << "final collection summed PDB sizes: " << collection_size << endl;
    }

    PatternCollectionInformation pattern_collection_information(
            task_proxy, patterns);
    pattern_collection_information.set_pdbs(pdbs);
    return pattern_collection_information;
}

void add_pattern_collection_generator_cegar_options_to_parser(options::OptionParser &parser) {
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
            Bounds("1", "infinity")
    );
    parser.add_option<int>(
            "max_collection_size",
            "limit for the total number of PDB entries across all PDBs (not "
            "applied to initial goal variable pattern(s))",
            "infinity",
            Bounds("1","infinity")
    );
    parser.add_option<bool>(
            "wildcard_plans",
            "Make the algorithm work with wildcard rather than regular plans.",
            "true"
    );
    parser.add_option<bool>(
            "ignore_goal_violations",
            "ignore goal violations and consequently generate a single pattern",
            "false");
    parser.add_option<int>(
            "global_blacklist_size",
            "Number of randomly selected non-goal variables that are globally "
            "blacklisted, which means excluded from being added to the pattern "
            "collection. 0 means no global blacklisting happens, infinity means "
            "to always exclude all non-goal variables.",
            "0",
            Bounds("0", "infinity")
    );
    std::vector<std::string> initial_collection_options;
    initial_collection_options.emplace_back("GIVEN_GOAL");
    initial_collection_options.emplace_back("RANDOM_GOAL");
    initial_collection_options.emplace_back("ALL_GOALS");
    parser.add_enum_option<InitialCollectionType>(
            "initial",
            initial_collection_options,
            "initial collection for refinement",
            "ALL_GOALS");
    parser.add_option<int>(
            "given_goal",
            "a goal variable to be used as the initial collection",
            "-1");
    parser.add_option<double>(
            "max_time",
            "maximum time in seconds for CEGAR pattern generation. "
            "This includes the creation of the initial PDB collection"
            " as well as the creation of the correlation matrix.",
            "infinity",
            Bounds("0.0", "infinity"));
    parser.add_option<bool>(
        "treat_goal_violations_differently",
        "If true, violated goal variables will be introduced as a separate "
        "pattern. Otherwise, they will be treated like precondition variables, "
        "thus added to the pattern in question or merging two patterns if "
        "already in the collection.",
        "true"
    );

    utils::add_verbosity_option_to_parser(parser);
}

static shared_ptr<PatternCollectionGenerator> _parse(
        options::OptionParser& parser) {
    add_pattern_collection_generator_cegar_options_to_parser(parser);
    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternCollectionGeneratorSingleCegar>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("single_cegar", _parse);
}
