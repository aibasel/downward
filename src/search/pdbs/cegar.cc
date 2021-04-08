#include "cegar.h"

#include "steepest_ascent_enforced_hill_climbing.h"
#include "types.h"
#include "utils.h"

#include "../option_parser.h"

#include "../tasks/projected_task_factory.h"

#include "../task_utils/task_properties.h"

#include "../utils/countdown_timer.h"
#include "../utils/rng.h"

using namespace std;

namespace pdbs {
void CEGAR::print_collection() const {
    utils::g_log << "[";
    for (size_t i = 0; i < projection_collection.size(); ++i) {
        const unique_ptr<Projection> &projection = projection_collection[i];
        if (projection) {
            utils::g_log << projection->get_pattern();
            if (i != projection_collection.size() - 1) {
                utils::g_log << ", ";
            }
        }
    }
    utils::g_log << "]" << endl;
}

bool CEGAR::time_limit_reached(
    const utils::CountdownTimer &timer) const {
    if (timer.is_expired()) {
        if (verbosity >= utils::Verbosity::NORMAL) {
            utils::g_log << "time limit reached." << endl;
        }
        return true;
    }
    return false;
}

unique_ptr<Projection> CEGAR::compute_projection(Pattern &&pattern) const {
    shared_ptr<PatternDatabase> pdb =
        make_shared<PatternDatabase>(task_proxy, pattern);
    shared_ptr<AbstractTask> projected_task =
        extra_tasks::build_projected_task(task, move(pattern));
    TaskProxy projected_task_proxy(*projected_task);

    bool unsolvable = false;
    vector<vector<OperatorID>> plan;
    size_t initial_state_index = pdb->hash_index_of_projected_state(
        projected_task_proxy.get_initial_state());
    int init_goal_dist = pdb->get_value_for_hash_index(initial_state_index);
    if (init_goal_dist == numeric_limits<int>::max()) {
        unsolvable = true;
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "PDB with pattern " << pdb->get_pattern()
                         << " is unsolvable" << endl;
        }
    } else {
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "Computing plan for PDB with pattern "
                         << pdb->get_pattern() << endl;
        }

        plan = steepest_ascent_enforced_hill_climbing(
            projected_task_proxy, rng, *pdb, wildcard_plans, verbosity);

        // Convert operator IDs of the abstract in the concrete task.
        for (vector<OperatorID> &plan_step : plan) {
            vector<OperatorID> concrete_plan_step;
            concrete_plan_step.reserve(plan_step.size());
            for (OperatorID abs_op_id : plan_step) {
                concrete_plan_step.push_back(
                    projected_task_proxy.get_operators()[abs_op_id].get_ancestor_operator_id(task.get()));
            }
            plan_step.swap(concrete_plan_step);
        }
    }

    return utils::make_unique_ptr<Projection>(
        move(pdb), move(plan), unsolvable);
}

void CEGAR::compute_initial_collection() {
    assert(!goals.empty());
    for (const FactPair &goal : goals) {
        add_pattern_for_var(goal.var);
    }

    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "initial collection: ";
        print_collection();
        utils::g_log << endl;
    }
}



/*
  TODO: this is a duplicate of State::get_unregistered_successor.
  In contrast to the original function, it does not assert that the given
  operator is applicable. It also does not evaluate axioms because this part
  of the code does not support axioms anyway.
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

FlawList CEGAR::get_violated_preconditions(
    int collection_index, const OperatorProxy &op, const State &state) const {
    FlawList flaws;
    for (FactProxy precondition : op.get_preconditions()) {
        int var = precondition.get_variable().get_id();

        // Ignore blacklisted variables.
        if (blacklisted_variables.count(var)) {
            continue;
        }

        if (state[precondition.get_variable()] != precondition) {
            flaws.emplace_back(collection_index, var);
        }
    }
    return flaws;
}

FlawList CEGAR::apply_plan(int collection_index, State &current) const {
    Projection &projection = *projection_collection[collection_index];
    const vector<vector<OperatorID>> &plan = projection.get_plan();
    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "executing plan for pattern "
                     << projection.get_pattern() << ": ";
    }
    for (const vector<OperatorID> &equivalent_ops : plan) {
        FlawList step_flaws;
        for (OperatorID op_id : equivalent_ops) {
            OperatorProxy op = task_proxy.get_operators()[op_id];
            FlawList operator_flaws = get_violated_preconditions(collection_index, op, current);

            /*
              If the operator is applicable, clear step_flaws, update the state
              to the successor state and proceed with the next plan step in
              the next iteration. Otherwise, move the flaws of the operator
              to step_flaws.
            */
            if (operator_flaws.empty()) {
                step_flaws.clear();
                current = get_unregistered_successor(task, current, op);
                break;
            } else {
                step_flaws.insert(step_flaws.end(),
                                  make_move_iterator(operator_flaws.begin()),
                                  make_move_iterator(operator_flaws.end()));
            }
        }

        /*
          If all equivalent operators are inapplicable, return the collected
          flaws of this plan step.
        */
        if (!step_flaws.empty()) {
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << "failure." << endl;
            }
            return step_flaws;
        }
    }

    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "success." << endl;
    }
    return {};
}

bool CEGAR::get_flaws_for_projection(
    int collection_index, const State &concrete_init, FlawList &flaws) {
    Projection &projection = *projection_collection[collection_index];
    if (projection.is_unsolvable()) {
        utils::g_log << "task is unsolvable." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSOLVABLE);
    }

    State current(concrete_init);
    FlawList new_flaws = apply_plan(collection_index, current);
    if (new_flaws.empty()) {
        if (task_properties::is_goal_state(task_proxy, current)) {
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << "plan led to a concrete goal state: ";
            }
            if (blacklisted_variables.empty()) {
                if (verbosity >= utils::Verbosity::VERBOSE) {
                    utils::g_log << "there are no blacklisted variables, "
                        "task solved." << endl;
                }
                return true;
            } else {
                if (verbosity >= utils::Verbosity::VERBOSE) {
                    utils::g_log << "there are blacklisted variables, "
                        "marking projection as solved." << endl;
                }
                projection.mark_as_solved();
            }
        } else {
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << "plan did not lead to a goal state: ";
            }
            bool raise_goal_flaw = false;
            for (const FactPair &goal : goals) {
                int goal_var_id = goal.var;
                if (current[goal_var_id].get_value() != goal.value &&
                    !blacklisted_variables.count(goal_var_id)) {
                    flaws.emplace_back(collection_index, goal_var_id);
                    raise_goal_flaw = true;
                }
            }
            if (raise_goal_flaw) {
                if (verbosity >= utils::Verbosity::VERBOSE) {
                    utils::g_log << "raising goal violation flaw(s)." << endl;
                }
            } else {
                if (verbosity >= utils::Verbosity::VERBOSE) {
                    utils::g_log << "there are no non-blacklisted goal variables "
                        "left, marking projection as solved." << endl;
                }
                projection.mark_as_solved();
            }
        }
    } else {
        flaws.insert(flaws.end(), make_move_iterator(new_flaws.begin()),
                     make_move_iterator(new_flaws.end()));
    }
    return false;
}

int CEGAR::get_flaws(const State &concrete_init, FlawList &flaws) {
    assert(flaws.empty());
    for (size_t collection_index = 0;
         collection_index < projection_collection.size(); ++collection_index) {
        if (projection_collection[collection_index] &&
            !projection_collection[collection_index]->is_solved()) {
            bool solved =
                get_flaws_for_projection(collection_index, concrete_init, flaws);
            if (solved) {
                return collection_index;
            }
        }
    }
    return -1;
}

void CEGAR::add_pattern_for_var(int var) {
    projection_collection.push_back(compute_projection({var}));
    variable_to_projection[var] = projection_collection.size() - 1;
    collection_size += projection_collection.back()->get_pdb()->get_size();
}

bool CEGAR::can_merge_patterns(int index1, int index2) const {
    int pdb_size1 = projection_collection[index1]->get_pdb()->get_size();
    int pdb_size2 = projection_collection[index2]->get_pdb()->get_size();
    if (!utils::is_product_within_limit(pdb_size1, pdb_size2, max_pdb_size)) {
        return false;
    }
    int added_size = pdb_size1 * pdb_size2 - pdb_size1 - pdb_size2;
    return collection_size + added_size <= max_collection_size;
}

void CEGAR::merge_patterns(int index1, int index2) {
    // Merge projection at index2 into projection at index2.
    Projection &projection1 = *projection_collection[index1];
    Projection &projection2 = *projection_collection[index2];

    const Pattern &pattern2 = projection2.get_pattern();
    for (int var : pattern2) {
        variable_to_projection[var] = index1;
    }

    // Compute merged pattern.
    Pattern new_pattern = projection1.get_pattern();
    new_pattern.insert(new_pattern.end(), pattern2.begin(), pattern2.end());
    sort(new_pattern.begin(), new_pattern.end());

    // Store old pdb sizes.
    int pdb_size1 = projection_collection[index1]->get_pdb()->get_size();
    int pdb_size2 = projection_collection[index2]->get_pdb()->get_size();

    // Compute merged projection.
    unique_ptr<Projection> merged = compute_projection(move(new_pattern));

    // Update collection size.
    collection_size -= pdb_size1;
    collection_size -= pdb_size2;
    collection_size += merged->get_pdb()->get_size();

    // Clean up.
    projection_collection[index1] = move(merged);
    projection_collection[index2] = nullptr;
}

bool CEGAR::can_add_variable_to_pattern(int index, int var) const {
    int pdb_size = projection_collection[index]->get_pdb()->get_size();
    int domain_size = task_proxy.get_variables()[var].get_domain_size();
    if (!utils::is_product_within_limit(pdb_size, domain_size, max_pdb_size)) {
        return false;
    }
    int added_size = pdb_size * domain_size - pdb_size;
    return collection_size + added_size <= max_collection_size;
}

void CEGAR::add_variable_to_pattern(int collection_index, int var) {
    const Projection &projection = *projection_collection[collection_index];

    Pattern new_pattern(projection.get_pattern());
    new_pattern.push_back(var);
    sort(new_pattern.begin(), new_pattern.end());

    unique_ptr<Projection> new_projection = compute_projection(move(new_pattern));

    collection_size -= projection.get_pdb()->get_size();
    collection_size += new_projection->get_pdb()->get_size();

    variable_to_projection[var] = collection_index;
    projection_collection[collection_index] = move(new_projection);
}

void CEGAR::refine(const FlawList &flaws) {
    assert(!flaws.empty());
    const Flaw &flaw = *(rng->choose(flaws));

    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "chosen flaw: pattern "
                     << projection_collection[flaw.collection_index]->get_pattern()
                     << " with a flaw on " << flaw.variable << endl;
    }

    int collection_index = flaw.collection_index;
    int var = flaw.variable;
    bool added_var = false;
    auto it = variable_to_projection.find(var);
    if (it != variable_to_projection.end()) {
        // Variable is contained in another pattern of the collection.
        int other_index = it->second;
        assert(other_index != collection_index);
        assert(projection_collection[other_index] != nullptr);
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "var" << var << " is already in pattern "
                         << projection_collection[other_index]->get_pattern() << endl;
        }
        if (can_merge_patterns(collection_index, other_index)) {
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << "merge the two patterns" << endl;
            }
            merge_patterns(collection_index, other_index);
            added_var = true;
        }
    } else {
        // Variable is not yet in the collection.
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "var" << var
                         << " is not in the collection yet" << endl;
        }
        if (can_add_variable_to_pattern(collection_index, var)) {
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << "add it to the pattern" << endl;
            }
            add_variable_to_pattern(collection_index, var);
            added_var = true;
        }
    }

    if (!added_var) {
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "could not add var/merge patterns due to size "
                "limits. Blacklisting." << endl;
        }
        blacklisted_variables.insert(var);
    }
}

PatternCollectionInformation CEGAR::compute_pattern_collection() {
    utils::CountdownTimer timer(max_time);
    compute_initial_collection();
    int iteration = 1;
    State concrete_init = task_proxy.get_initial_state();
    concrete_init.unpack();
    int concrete_solution_index = -1;
    while (!time_limit_reached(timer)) {
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "iteration #" << iteration << endl;
        }

        FlawList flaws;
        concrete_solution_index = get_flaws(concrete_init, flaws);

        if (concrete_solution_index != -1) {
            if (verbosity >= utils::Verbosity::NORMAL) {
                utils::g_log << "task solved during computation of abstraction"
                             << endl;
            }
            break;
        }

        if (flaws.empty()) {
            if (verbosity >= utils::Verbosity::NORMAL) {
                utils::g_log << "flaw list empty. No further refinements possible"
                             << endl;
            }
            break;
        }

        refine(flaws);
        ++iteration;

        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "current collection size: " << collection_size << endl;
            utils::g_log << "current collection: ";
            print_collection();
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
            projection_collection[concrete_solution_index]->get_pdb();
        patterns->push_back(pdb->get_pattern());
        pdbs->push_back(pdb);
    } else {
        for (const unique_ptr<Projection> &projection : projection_collection) {
            if (projection) {
                const shared_ptr<PatternDatabase> &pdb = projection->get_pdb();
                patterns->push_back(pdb->get_pattern());
                pdbs->push_back(pdb);
            }
        }
    }

    PatternCollectionInformation pattern_collection_information(
        task_proxy, patterns);
    pattern_collection_information.set_pdbs(pdbs);

    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << "CEGAR number of iterations: " << iteration << endl;
        dump_pattern_collection_generation_statistics(
            "CEGAR",
            timer.get_elapsed_time(),
            pattern_collection_information,
            collection_size);
    }

    return pattern_collection_information;
}

CEGAR::CEGAR(
    int max_pdb_size,
    int max_collection_size,
    bool wildcard_plans,
    double max_time,
    utils::Verbosity verbosity,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const shared_ptr<AbstractTask> &task,
    vector<FactPair> &&goals,
    unordered_set<int> &&blacklisted_variables)
    : max_pdb_size(max_pdb_size),
      max_collection_size(max_collection_size),
      wildcard_plans(wildcard_plans),
      max_time(max_time),
      verbosity(verbosity),
      rng(rng),
      task(task),
      task_proxy(*task),
      goals(move(goals)),
      blacklisted_variables(move(blacklisted_variables)),
      collection_size(0) {
#ifndef NDEBUG
    for (const FactPair &goal : goals) {
        bool is_goal = false;
        for (FactProxy task_goal : task_proxy.get_goals()) {
            if (goal == task_goal.get_pair()) {
                is_goal = true;
                break;
            }
        }
        if (!is_goal) {
            cerr << "given goal is not a goal of the task." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
    }
#endif
    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << "options of the CEGAR algorithm for computing a pattern collection: " << endl;
        utils::g_log << "max pdb size: " << max_pdb_size << endl;
        utils::g_log << "max collection size: " << max_collection_size << endl;
        utils::g_log << "wildcard plans: " << wildcard_plans << endl;
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
        utils::g_log << "goal variables: ";
        for (const FactPair &goal : goals) {
            utils::g_log << goal.var << ", ";
        }
        utils::g_log << endl;
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
}

void add_cegar_options_to_parser(options::OptionParser &parser) {
    parser.add_option<int>(
        "max_pdb_size",
        "maximum number of states per pattern database (ignored for the "
        "initial collection consisting of singleton patterns for each goal "
        "variable)",
        "2000000",
        Bounds("1", "infinity"));
    parser.add_option<int>(
        "max_collection_size",
        "maximum number of states in the pattern collection (ignored for the "
        "initial collection consisting of singleton patterns for each goal "
        "variable)",
        "20000000",
        Bounds("1", "infinity"));
    parser.add_option<bool>(
        "wildcard_plans",
        "if true, compute wildcard plans which are sequences of sets of "
        "operators that induce the same transition; otherwise compute regular "
        "plans which are sequences of single operators",
        "true");
    parser.add_option<double>(
        "max_time",
        "maximum time in seconds for the CEGAR algorithm (ignored for"
        "computing initial collection)",
        "infinity",
        Bounds("0.0", "infinity"));
}
}
