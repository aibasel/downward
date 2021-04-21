#include "cegar.h"

#include "abstract_operator.h"
#include "match_tree.h"
#include "pattern_database.h"
#include "pattern_database_factory.h"
#include "steepest_ascent_enforced_hill_climbing.h"
#include "types.h"
#include "utils.h"

#include "../option_parser.h"

#include "../task_utils/task_properties.h"

#include "../utils/countdown_timer.h"
#include "../utils/rng.h"

using namespace std;

namespace pdbs {
void CEGAR::print_collection() const {
    utils::g_log << "[";
    for (size_t i = 0; i < collection.size(); ++i) {
        const unique_ptr<PDBInfo> &collection_entry = collection[i];
        if (collection_entry) {
            utils::g_log << collection_entry->get_pattern();
            if (i != collection.size() - 1) {
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

unique_ptr<PDBInfo> CEGAR::compute_pdb_and_plan(Pattern &&pattern) const {
    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "Computing PDB and plan for pattern "
                     << pattern << endl;
    }

    Projection projection(task_proxy, pattern);
    PerfectHashFunction hash_function = compute_hash_function(
        task_proxy, pattern);
    OperatorType operator_type(OperatorType::RegressionAndProgression);
    bool compute_op_id_mapping = true;
    AbstractOperators abstract_operators(
        projection,
        hash_function,
        operator_type,
        {},
        compute_op_id_mapping);
    const vector<AbstractOperator> &regression_operators = abstract_operators.get_regression_operators();
    MatchTree regression_match_tree = build_match_tree(
        projection,
        hash_function,
        regression_operators);
//    cout << "regression operators" << endl;
//    for (const AbstractOperator &op : regression_operators) {
//        op.dump(projection.get_pattern(), projection.get_task_proxy().get_variables());
//    }
//    cout << "regression match tree" << endl;
//    regression_match_tree.dump();
    vector<int> distances = compute_distances(
        projection,
        hash_function,
        regression_operators,
        regression_match_tree);

    State initial_state = projection.get_task_proxy().get_initial_state();
    initial_state.unpack();
    size_t initial_state_hash = hash_function.rank(initial_state.get_unpacked_values());
    int init_goal_dist = distances[initial_state_hash];
    bool unsolvable;
    vector<vector<OperatorID>> plan;
    if (init_goal_dist == numeric_limits<int>::max()) {
        unsolvable = true;
    } else {
        unsolvable = false;
        plan = steepest_ascent_enforced_hill_climbing(
            projection, hash_function, distances, abstract_operators, rng,
            wildcard_plans, verbosity, initial_state_hash);
    }
    if (verbosity >= utils::Verbosity::VERBOSE) {
        if (unsolvable) {
            utils::g_log << "PDB is unsolvable" << endl;
        } else {
            utils::g_log << "##### Start of plan #####" << endl;
            int step = 1;
            for (const vector<OperatorID> &equivalent_ops : plan) {
                utils::g_log << "step #" << step << endl;
                for (OperatorID op_id : equivalent_ops) {
                    OperatorProxy op = task_proxy.get_operators()[op_id];
                    utils::g_log << op.get_name() << " " << op.get_cost() << endl;
                }
                ++step;
            }
            utils::g_log << "##### End of plan #####" << endl;
        }
    }

    return utils::make_unique_ptr<PDBInfo>(
        make_shared<PatternDatabase>(move(hash_function), move(distances)),
        move(plan),
        unsolvable);
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
  Apply the operator to the given state, ignoring that the operator is
  potentially not applicable in the state, and expecting that the operator
  has not conditional effects.
*/
void apply_op_to_state(vector<int> &state, const OperatorProxy &op) {
    assert(!op.is_axiom());
    for (EffectProxy effect : op.get_effects()) {
        assert(effect.get_conditions().empty());
        FactPair effect_fact = effect.get_fact().get_pair();
        state[effect_fact.var] = effect_fact.value;
    }
}

FlawList CEGAR::get_violated_preconditions(
    int collection_index,
    const OperatorProxy &op,
    const vector<int> &current_state) const {
    FlawList flaws;
    for (FactProxy precondition : op.get_preconditions()) {
        int var_id = precondition.get_variable().get_id();

        // Ignore blacklisted variables.
        if (blacklisted_variables.count(var_id)) {
            continue;
        }

        if (current_state[var_id] != precondition.get_value()) {
            flaws.emplace_back(collection_index, var_id);
        }
    }
    return flaws;
}

FlawList CEGAR::apply_plan(int collection_index, vector<int> &current_state) const {
    PDBInfo &pdb_info = *collection[collection_index];
    const vector<vector<OperatorID>> &plan = pdb_info.get_plan();
    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "executing plan for pattern "
                     << pdb_info.get_pattern() << ": ";
    }
    for (const vector<OperatorID> &equivalent_ops : plan) {
        FlawList step_flaws;
        for (OperatorID op_id : equivalent_ops) {
            OperatorProxy op = task_proxy.get_operators()[op_id];
            FlawList operator_flaws = get_violated_preconditions(collection_index, op, current_state);

            /*
              If the operator is applicable, clear step_flaws, update the state
              to the successor state and proceed with the next plan step in
              the next iteration. Otherwise, move the flaws of the operator
              to step_flaws.
            */
            if (operator_flaws.empty()) {
                step_flaws.clear();
                apply_op_to_state(current_state, op);
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

bool CEGAR::get_flaws_for_pdb(
    int collection_index, const State &concrete_init, FlawList &flaws) {
    PDBInfo &collection_entry = *collection[collection_index];
    if (collection_entry.is_unsolvable()) {
        utils::g_log << "task is unsolvable." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSOLVABLE);
    }

    vector<int> current_state = concrete_init.get_unpacked_values();
    FlawList new_flaws = apply_plan(collection_index, current_state);
    if (new_flaws.empty()) {
        State final_state(*task, move(current_state));
        if (task_properties::is_goal_state(task_proxy, final_state)) {
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
                        "marking collection_entry as solved." << endl;
                }
                collection_entry.mark_as_solved();
            }
        } else {
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << "plan did not lead to a goal state: ";
            }
            bool raise_goal_flaw = false;
            for (const FactPair &goal : goals) {
                int goal_var_id = goal.var;
                if (final_state[goal_var_id].get_value() != goal.value &&
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
                        "left, marking collection_entry as solved." << endl;
                }
                collection_entry.mark_as_solved();
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
         collection_index < collection.size(); ++collection_index) {
        if (collection[collection_index] &&
            !collection[collection_index]->is_solved()) {
            bool solved =
                get_flaws_for_pdb(collection_index, concrete_init, flaws);
            if (solved) {
                return collection_index;
            }
        }
    }
    return -1;
}

void CEGAR::add_pattern_for_var(int var) {
    collection.push_back(compute_pdb_and_plan({var}));
    variable_to_collection_index[var] = collection.size() - 1;
    collection_size += collection.back()->get_pdb()->get_size();
}

bool CEGAR::can_merge_patterns(int index1, int index2) const {
    int pdb_size1 = collection[index1]->get_pdb()->get_size();
    int pdb_size2 = collection[index2]->get_pdb()->get_size();
    if (!utils::is_product_within_limit(pdb_size1, pdb_size2, max_pdb_size)) {
        return false;
    }
    int added_size = pdb_size1 * pdb_size2 - pdb_size1 - pdb_size2;
    return collection_size + added_size <= max_collection_size;
}

void CEGAR::merge_patterns(int index1, int index2) {
    // Merge pattern at index2 into pattern at index2.
    PDBInfo &pdb_info1 = *collection[index1];
    PDBInfo &pdb_info2 = *collection[index2];

    const Pattern &pattern2 = pdb_info2.get_pattern();
    for (int var : pattern2) {
        variable_to_collection_index[var] = index1;
    }

    // Compute merged_pdb_info pattern.
    Pattern new_pattern = pdb_info1.get_pattern();
    new_pattern.insert(new_pattern.end(), pattern2.begin(), pattern2.end());
    sort(new_pattern.begin(), new_pattern.end());

    // Store old pdb sizes.
    int pdb_size1 = collection[index1]->get_pdb()->get_size();
    int pdb_size2 = collection[index2]->get_pdb()->get_size();

    // Compute PDB and plan for merged_pdb_info pattern.
    unique_ptr<PDBInfo> merged_pdb_info = compute_pdb_and_plan(move(new_pattern));

    // Update collection size.
    collection_size -= pdb_size1;
    collection_size -= pdb_size2;
    collection_size += merged_pdb_info->get_pdb()->get_size();

    // Clean up.
    collection[index1] = move(merged_pdb_info);
    collection[index2] = nullptr;
}

bool CEGAR::can_add_variable_to_pattern(int index, int var) const {
    int pdb_size = collection[index]->get_pdb()->get_size();
    int domain_size = task_proxy.get_variables()[var].get_domain_size();
    if (!utils::is_product_within_limit(pdb_size, domain_size, max_pdb_size)) {
        return false;
    }
    int added_size = pdb_size * domain_size - pdb_size;
    return collection_size + added_size <= max_collection_size;
}

void CEGAR::add_variable_to_pattern(int collection_index, int var) {
    const PDBInfo &pdb_info = *collection[collection_index];

    Pattern new_pattern(pdb_info.get_pattern());
    new_pattern.push_back(var);
    sort(new_pattern.begin(), new_pattern.end());

    unique_ptr<PDBInfo> new_pdb_info = compute_pdb_and_plan(move(new_pattern));

    collection_size -= pdb_info.get_pdb()->get_size();
    collection_size += new_pdb_info->get_pdb()->get_size();

    variable_to_collection_index[var] = collection_index;
    collection[collection_index] = move(new_pdb_info);
}

void CEGAR::refine(const FlawList &flaws) {
    assert(!flaws.empty());
    const Flaw &flaw = *(rng->choose(flaws));

    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "chosen flaw: pattern "
                     << collection[flaw.collection_index]->get_pattern()
                     << " with a flaw on " << flaw.variable << endl;
    }

    int collection_index = flaw.collection_index;
    int var = flaw.variable;
    bool added_var = false;
    auto it = variable_to_collection_index.find(var);
    if (it != variable_to_collection_index.end()) {
        // Variable is contained in another pattern of the collection.
        int other_index = it->second;
        assert(other_index != collection_index);
        assert(collection[other_index] != nullptr);
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "var" << var << " is already in pattern "
                         << collection[other_index]->get_pattern() << endl;
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
            collection[concrete_solution_index]->get_pdb();
        patterns->push_back(pdb->get_pattern());
        pdbs->push_back(pdb);
    } else {
        for (const unique_ptr<PDBInfo> &pdb_info : collection) {
            if (pdb_info) {
                const shared_ptr<PatternDatabase> &pdb = pdb_info->get_pdb();
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

//    for (VariableProxy var : task_proxy.get_variables()) {
//        cout << "var " << var.get_id() << ": ";
//        for (int fact_id = 0; fact_id < var.get_domain_size(); ++fact_id) {
//            cout << var.get_fact(fact_id).get_pair() << ", ";
//            cout << var.get_fact(fact_id).get_name() << "; ";
//        }
//        cout << endl;
//    }
//
//    for (OperatorProxy op : task_proxy.get_operators()) {
//        cout << "op " << op.get_id() << ", " << op.get_name() << endl;
//        cout << "pre: ";
//        for (FactProxy pre : op.get_preconditions()) {
//            cout << pre.get_pair() << ", ";
//            cout << pre.get_name() << "; ";
//        }
//        cout << endl;
//        cout << "eff: ";
//        for (EffectProxy eff : op.get_effects()) {
//            cout << eff.get_fact().get_pair() << ", ";
//            cout << eff.get_fact().get_name() << "; ";
//        }
//        cout << endl;
//    }
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
