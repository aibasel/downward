#include "cegar.h"

#include "types.h"
#include "utils.h"

#include "../option_parser.h"

#include "../task_utils/task_properties.h"

#include "../utils/countdown_timer.h"
#include "../utils/rng.h"

#include <limits>

using namespace std;

namespace pdbs {
CEGAR::CEGAR(
    int max_pdb_size,
    int max_collection_size,
    bool use_wildcard_plans,
    double max_time,
    utils::Verbosity verbosity,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const shared_ptr<AbstractTask> &task,
    vector<FactPair> &&goals,
    unordered_set<int> &&blacklisted_variables)
    : max_pdb_size(max_pdb_size),
      max_collection_size(max_collection_size),
      use_wildcard_plans(use_wildcard_plans),
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
        utils::g_log << "wildcard plans: " << use_wildcard_plans << endl;
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
        for (const FactPair &goal : this->goals) {
            utils::g_log << goal.var << ", ";
        }
        utils::g_log << endl;
        utils::g_log << "blacklisted variables: ";
        if (this->blacklisted_variables.empty()) {
            utils::g_log << "none";
        } else {
            for (int var : this->blacklisted_variables) {
                utils::g_log << var << ", ";
            }
        }
        utils::g_log << endl;
    }
}

void CEGAR::print_collection() const {
    utils::g_log << "[";
    for (size_t i = 0; i < pattern_collection.size(); ++i) {
        const unique_ptr<PatternInfo> &pattern_info = pattern_collection[i];
        if (pattern_info) {
            utils::g_log << pattern_info->get_pattern();
            if (i != pattern_collection.size() - 1) {
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

unique_ptr<PatternInfo> CEGAR::compute_pattern_info(Pattern &&pattern) const {
    bool dump = false;
    vector<int> op_cost;
    bool compute_plan = true;
    shared_ptr<PatternDatabase> pdb =
        make_shared<PatternDatabase>(task_proxy, pattern, dump, op_cost, compute_plan, rng, use_wildcard_plans);
    vector<vector<OperatorID>> plan = pdb->extract_wildcard_plan();

    bool unsolvable = false;
    State initial_state = task_proxy.get_initial_state();
    initial_state.unpack();
    if (pdb->get_value(initial_state.get_unpacked_values()) == numeric_limits<int>::max()) {
        unsolvable = true;
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "Projection onto pattern " << pdb->get_pattern()
                         << " is unsolvable" << endl;
        }
    } else {
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "##### Plan for pattern " << pdb->get_pattern() << " #####" << endl;
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
    return utils::make_unique_ptr<PatternInfo>(move(pdb), move(plan), unsolvable);
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
    PatternInfo &pattern_info = *pattern_collection[collection_index];
    const vector<vector<OperatorID>> &plan = pattern_info.get_plan();
    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "executing plan for pattern "
                     << pattern_info.get_pattern() << ": ";
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

bool CEGAR::get_flaws_for_pattern(
    int collection_index, const State &concrete_init, FlawList &flaws) {
    PatternInfo &pattern_info = *pattern_collection[collection_index];
    if (pattern_info.is_unsolvable()) {
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
                        "marking pattern as solved." << endl;
                }
                pattern_info.mark_as_solved();
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
                        "left, marking pattern as solved." << endl;
                }
                pattern_info.mark_as_solved();
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
         collection_index < pattern_collection.size(); ++collection_index) {
        if (pattern_collection[collection_index] &&
            !pattern_collection[collection_index]->is_solved()) {
            bool solved =
                get_flaws_for_pattern(collection_index, concrete_init, flaws);
            if (solved) {
                return collection_index;
            }
        }
    }
    return -1;
}

void CEGAR::add_pattern_for_var(int var) {
    pattern_collection.push_back(compute_pattern_info({var}));
    variable_to_collection_index[var] = pattern_collection.size() - 1;
    collection_size += pattern_collection.back()->get_pdb()->get_size();
}

bool CEGAR::can_merge_patterns(int index1, int index2) const {
    int pdb_size1 = pattern_collection[index1]->get_pdb()->get_size();
    int pdb_size2 = pattern_collection[index2]->get_pdb()->get_size();
    if (!utils::is_product_within_limit(pdb_size1, pdb_size2, max_pdb_size)) {
        return false;
    }
    int added_size = pdb_size1 * pdb_size2 - pdb_size1 - pdb_size2;
    return collection_size + added_size <= max_collection_size;
}

void CEGAR::merge_patterns(int index1, int index2) {
    // Merge pattern at index2 into pattern at index2.
    PatternInfo &pattern_info1 = *pattern_collection[index1];
    PatternInfo &pattern_info2 = *pattern_collection[index2];

    const Pattern &pattern2 = pattern_info2.get_pattern();
    for (int var : pattern2) {
        variable_to_collection_index[var] = index1;
    }

    // Compute merged_pattern_info pattern.
    Pattern new_pattern = pattern_info1.get_pattern();
    new_pattern.insert(new_pattern.end(), pattern2.begin(), pattern2.end());
    sort(new_pattern.begin(), new_pattern.end());

    // Store old PDB sizes.
    int pdb_size1 = pattern_collection[index1]->get_pdb()->get_size();
    int pdb_size2 = pattern_collection[index2]->get_pdb()->get_size();

    // Compute merged_pattern_info pattern.
    unique_ptr<PatternInfo> merged_pattern_info = compute_pattern_info(move(new_pattern));

    // Update collection size.
    collection_size -= pdb_size1;
    collection_size -= pdb_size2;
    collection_size += merged_pattern_info->get_pdb()->get_size();

    // Clean up.
    pattern_collection[index1] = move(merged_pattern_info);
    pattern_collection[index2] = nullptr;
}

bool CEGAR::can_add_variable_to_pattern(int index, int var) const {
    int pdb_size = pattern_collection[index]->get_pdb()->get_size();
    int domain_size = task_proxy.get_variables()[var].get_domain_size();
    if (!utils::is_product_within_limit(pdb_size, domain_size, max_pdb_size)) {
        return false;
    }
    int added_size = pdb_size * domain_size - pdb_size;
    return collection_size + added_size <= max_collection_size;
}

void CEGAR::add_variable_to_pattern(int collection_index, int var) {
    const PatternInfo &pattern_info = *pattern_collection[collection_index];

    Pattern new_pattern(pattern_info.get_pattern());
    new_pattern.push_back(var);
    sort(new_pattern.begin(), new_pattern.end());

    unique_ptr<PatternInfo> new_pattern_info = compute_pattern_info(move(new_pattern));

    collection_size -= pattern_info.get_pdb()->get_size();
    collection_size += new_pattern_info->get_pdb()->get_size();

    variable_to_collection_index[var] = collection_index;
    pattern_collection[collection_index] = move(new_pattern_info);
}

void CEGAR::refine(const FlawList &flaws) {
    assert(!flaws.empty());
    const Flaw &flaw = *(rng->choose(flaws));

    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "chosen flaw: pattern "
                     << pattern_collection[flaw.collection_index]->get_pattern()
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
        assert(pattern_collection[other_index] != nullptr);
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "var" << var << " is already in pattern "
                         << pattern_collection[other_index]->get_pattern() << endl;
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
            pattern_collection[concrete_solution_index]->get_pdb();
        patterns->push_back(pdb->get_pattern());
        pdbs->push_back(pdb);
    } else {
        for (const unique_ptr<PatternInfo> &pattern_info : pattern_collection) {
            if (pattern_info) {
                const shared_ptr<PatternDatabase> &pdb = pattern_info->get_pdb();
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
            pattern_collection_information);
    }

    return pattern_collection_information;
}

void add_implementation_notes_to_parser(options::OptionParser &parser) {
    parser.document_note(
        "Implementation Notes",
        "The following describes differences of the implementation to "
        "the original implementation used and described in the paper.\n\n"
        "Conceptually, there is one larger difference which concern the "
        "computation of (regular or wildcard) plans for PDBs. The original "
        "implementation used an enforced hill-climbing search with the PDB "
        "as the perfect heuristic. To handle domains with zero-cost operators, "
        "it used a breadth-first search in each iteration to find the best "
        "improving successor. This ensured to compute strongly optimal plans, "
        "i.e., optimal plans with a minimum number of zero-cost operators. "
        "The original implementation also ensured to use uniform random tie-"
        "breaking when choosing the best successor.\n\n"
        "In contrast, the current implementation computes a plan alongside the "
        "computation of the PDB itself. A modification to Dijkstra's algorithm "
        "for computing the PDB values stores, for each state, the operator "
        "leading to that state (in a regression search). This generating "
        "operator is updated only if the algorithm found a better value for the "
        "state. After Dijkstra's finishes, the plan computation starts at the "
        "initial state and iteratively follows the generating operator, computes "
        "all operators of the same cost inducing the same transition, until "
        "reaching a goal. This constitutes a wildcard plan. It it turned into a "
        "regular one by randomly picking a single operator for each transition. "
        "\n\n"
        "Note that this kind of plan extraction does not uniformly at random "
        "consider all successor of a state but rather uses the arbitrarily "
        "chosen generating operator to settle on one successor state, which is "
        "biased by the number of operators leading to the same successor from "
        "the given state. Further note that in the presence of zero-cost "
        "operators, this procedure does not guarantee that the computed plan is "
        "strongly optimal because it does not minimize the number of used "
        "zero-cost operators leading to the state when choosing a generating "
        "operator. Experiments have shown (issue1007) that this speeds up the "
        "computation significantly while not having a strongly negative effect "
        "on heuristic quality due to potentially computing worse plans.\n\n"
        "Two further changes fix bugs of the original implementation to match "
        "the description in the paper. The first concerns the single CEGAR "
        "algorithm: Raise a flaw for all goal variables of the task if the "
        "plan for PDB can be executed on the concrete task but does not lead "
        "to a goal state. Previously, such flaws would not have been raised "
        "because all goal variables are part of the collection from the start "
        "on and therefore not considered. This means that the original "
        "implementation accidentally disallowed merging patterns due to goal "
        "violation flaws. The second bug fix is to actually randomize the "
        "order of parallel operators in wildcard plan steps.",
        true);
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
        "use_wildcard_plans",
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
