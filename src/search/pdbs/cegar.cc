#include "cegar.h"

#include "types.h"
#include "utils.h"

#include "../option_parser.h"
#include "../task_proxy.h"

#include "../task_utils/task_properties.h"

#include "../utils/countdown_timer.h"
#include "../utils/logging.h"
#include "../utils/rng.h"

#include <limits>
#include <unordered_set>

using namespace std;

namespace pdbs {
/*
  This is used as a "collection entry" in the CEGAR algorithm. It stores
  the PDB (and with that, the pattern) and an optimal plan (in the wildcard
  format) for that PDB if it exists or unsolvable is true otherwise. It can
  be marked as "solved" to ignore it in further iterations of the CEGAR
  algorithm.
*/
class PatternInfo {
    shared_ptr<PatternDatabase> pdb;
    vector<vector<OperatorID>> plan;
    bool unsolvable;
    bool solved;

public:
    PatternInfo(
        const shared_ptr<PatternDatabase> &&pdb,
        const vector<vector<OperatorID>> &&plan,
        bool unsolvable)
        : pdb(move(pdb)),
          plan(move(plan)),
          unsolvable(unsolvable),
          solved(false) {}

    const shared_ptr<PatternDatabase> &get_pdb() const {
        return pdb;
    }

    const Pattern &get_pattern() const {
        return pdb->get_pattern();
    }

    const vector<vector<OperatorID>> &get_plan() const {
        return plan;
    }

    bool is_unsolvable() const {
        return unsolvable;
    }

    void mark_as_solved() {
        solved = true;
    }

    bool is_solved() {
        return solved;
    }
};

struct Flaw {
    int collection_index;
    int variable;

    Flaw(int collection_index, int variable)
        : collection_index(collection_index),
          variable(variable) {
    }
};

using FlawList = vector<Flaw>;

class CEGAR {
    const int max_pdb_size;
    const int max_collection_size;
    const double max_time;
    const bool use_wildcard_plans;
    utils::LogProxy &log;
    shared_ptr<utils::RandomNumberGenerator> rng;
    const shared_ptr<AbstractTask> &task;
    const TaskProxy task_proxy;
    const vector<FactPair> &goals;
    unordered_set<int> blacklisted_variables;

    vector<unique_ptr<PatternInfo>> pattern_collection;
    /*
      Map each variable of the task which is contained in the collection to the
      collection index at which the pattern containing the variable is stored.
    */
    unordered_map<int, int> variable_to_collection_index;
    int collection_size;

    void print_collection() const;
    bool time_limit_reached(const utils::CountdownTimer &timer) const;

    unique_ptr<PatternInfo> compute_pattern_info(Pattern &&pattern) const;
    void compute_initial_collection();

    /*
      Check if operator op is applicable in state, ignoring blacklisted
      variables. If it is, return an empty (flaw) list. Otherwise, return the
      violated preconditions.
    */
    FlawList get_violated_preconditions(
        int collection_index,
        const OperatorProxy &op,
        const vector<int> &current_state) const;
    /*
      Try to apply the plan of the pattern at the given index in the
      concrete task starting at the given state. During application,
      blacklisted variables are ignored. If plan application succeeds,
      return an empty flaw list. Otherwise, return all precondition variables
      of all operators of the failing plan step. When the method returns,
      current is the last state reached when executing the plan.
     */
    FlawList apply_plan(int collection_index, vector<int> &current_state) const;
    /*
      Use apply_plan to generate flaws. Return true if there are no flaws and
      no blacklisted variables, in which case the concrete task is solved.
      Return false in all other cases. Append new flaws to the passed-in flaws.
    */
    bool get_flaws_for_pattern(
        int collection_index, const State &concrete_init, FlawList &flaws);
    /*
      Use get_flaws_for_pattern for all patterns of the collection. Append
      new flaws to the passed-in flaws. If the task is solved by the plan of
      any pattern, return the collection index of that pattern. Otherwise,
      return -1.
    */
    int get_flaws(const State &concrete_init, FlawList &flaws);

    // Methods related to refining.
    void add_pattern_for_var(int var);
    bool can_merge_patterns(int index1, int index2) const;
    void merge_patterns(int index1, int index2);
    bool can_add_variable_to_pattern(int index, int var) const;
    void add_variable_to_pattern(int collection_index, int var);
    void refine(const FlawList &flaws);
public:
    CEGAR(
        int max_pdb_size,
        int max_collection_size,
        double max_time,
        bool use_wildcard_plans,
        utils::LogProxy &log,
        const shared_ptr<utils::RandomNumberGenerator> &rng,
        const shared_ptr<AbstractTask> &task,
        const vector<FactPair> &goals,
        unordered_set<int> &&blacklisted_variables = unordered_set<int>());
    PatternCollectionInformation compute_pattern_collection();
};

CEGAR::CEGAR(
    int max_pdb_size,
    int max_collection_size,
    double max_time,
    bool use_wildcard_plans,
    utils::LogProxy &log,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const shared_ptr<AbstractTask> &task,
    const vector<FactPair> &goals,
    unordered_set<int> &&blacklisted_variables)
    : max_pdb_size(max_pdb_size),
      max_collection_size(max_collection_size),
      max_time(max_time),
      use_wildcard_plans(use_wildcard_plans),
      log(log),
      rng(rng),
      task(task),
      task_proxy(*task),
      goals(goals),
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
}

void CEGAR::print_collection() const {
    if (log.is_at_least_verbose()) {
        log << "[";
        for (size_t i = 0; i < pattern_collection.size(); ++i) {
            const unique_ptr<PatternInfo> &pattern_info = pattern_collection[i];
            if (pattern_info) {
                log << pattern_info->get_pattern();
                if (i != pattern_collection.size() - 1) {
                    log << ", ";
                }
            }
        }
        log << "]" << endl;
    }
}

bool CEGAR::time_limit_reached(
    const utils::CountdownTimer &timer) const {
    if (timer.is_expired()) {
        if (log.is_at_least_normal()) {
            log << "CEGAR time limit reached" << endl;
        }
        return true;
    }
    return false;
}

unique_ptr<PatternInfo> CEGAR::compute_pattern_info(Pattern &&pattern) const {
    vector<int> op_cost;
    bool compute_plan = true;
    shared_ptr<PatternDatabase> pdb =
        make_shared<PatternDatabase>(task_proxy, pattern, op_cost, compute_plan, rng, use_wildcard_plans);
    vector<vector<OperatorID>> plan = pdb->extract_wildcard_plan();

    bool unsolvable = false;
    State initial_state = task_proxy.get_initial_state();
    initial_state.unpack();
    if (pdb->get_value(initial_state.get_unpacked_values()) == numeric_limits<int>::max()) {
        unsolvable = true;
        if (log.is_at_least_verbose()) {
            log << "projection onto pattern " << pdb->get_pattern()
                << " is unsolvable" << endl;
        }
    } else {
        if (log.is_at_least_verbose()) {
            log << "##### Plan for pattern " << pdb->get_pattern() << " #####" << endl;
            int step = 1;
            for (const vector<OperatorID> &equivalent_ops : plan) {
                log << "step #" << step << endl;
                for (OperatorID op_id : equivalent_ops) {
                    OperatorProxy op = task_proxy.get_operators()[op_id];
                    log << op.get_name() << " " << op.get_cost() << endl;
                }
                ++step;
            }
            log << "##### End of plan #####" << endl;
        }
    }
    return utils::make_unique_ptr<PatternInfo>(move(pdb), move(plan), unsolvable);
}

void CEGAR::compute_initial_collection() {
    assert(!goals.empty());
    for (const FactPair &goal : goals) {
        add_pattern_for_var(goal.var);
    }

    if (log.is_at_least_verbose()) {
        log << "initial collection: ";
        print_collection();
        log << endl;
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
    if (log.is_at_least_verbose()) {
        log << "executing plan for pattern "
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
            if (log.is_at_least_verbose()) {
                log << "failure." << endl;
            }
            return step_flaws;
        }
    }

    if (log.is_at_least_verbose()) {
        log << "success." << endl;
    }
    return {};
}

bool CEGAR::get_flaws_for_pattern(
    int collection_index, const State &concrete_init, FlawList &flaws) {
    PatternInfo &pattern_info = *pattern_collection[collection_index];
    if (pattern_info.is_unsolvable()) {
        log << "task is unsolvable." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSOLVABLE);
    }

    vector<int> current_state = concrete_init.get_unpacked_values();
    FlawList new_flaws = apply_plan(collection_index, current_state);
    if (new_flaws.empty()) {
        State final_state(*task, move(current_state));
        if (task_properties::is_goal_state(task_proxy, final_state)) {
            if (log.is_at_least_verbose()) {
                log << "plan led to a concrete goal state: ";
            }
            if (blacklisted_variables.empty()) {
                if (log.is_at_least_verbose()) {
                    log << "there are no blacklisted variables, "
                        "task solved." << endl;
                }
                return true;
            } else {
                if (log.is_at_least_verbose()) {
                    log << "there are blacklisted variables, "
                        "marking pattern as solved." << endl;
                }
                pattern_info.mark_as_solved();
            }
        } else {
            if (log.is_at_least_verbose()) {
                log << "plan did not lead to a goal state: ";
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
                if (log.is_at_least_verbose()) {
                    log << "raising goal violation flaw(s)." << endl;
                }
            } else {
                if (log.is_at_least_verbose()) {
                    log << "there are no non-blacklisted goal variables "
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

    if (log.is_at_least_verbose()) {
        log << "chosen flaw: pattern "
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
        if (log.is_at_least_verbose()) {
            log << "var" << var << " is already in pattern "
                << pattern_collection[other_index]->get_pattern() << endl;
        }
        if (can_merge_patterns(collection_index, other_index)) {
            if (log.is_at_least_verbose()) {
                log << "merge the two patterns" << endl;
            }
            merge_patterns(collection_index, other_index);
            added_var = true;
        }
    } else {
        // Variable is not yet in the collection.
        if (log.is_at_least_verbose()) {
            log << "var" << var
                << " is not in the collection yet" << endl;
        }
        if (can_add_variable_to_pattern(collection_index, var)) {
            if (log.is_at_least_verbose()) {
                log << "add it to the pattern" << endl;
            }
            add_variable_to_pattern(collection_index, var);
            added_var = true;
        }
    }

    if (!added_var) {
        if (log.is_at_least_verbose()) {
            log << "could not add var/merge pattern containing var "
                << "due to size limits, blacklisting var" << endl;
        }
        blacklisted_variables.insert(var);
    }
}

PatternCollectionInformation CEGAR::compute_pattern_collection() {
    if (log.is_at_least_normal()) {
        log << "CEGAR options:" << endl;
        log << "max pdb size: " << max_pdb_size << endl;
        log << "max collection size: " << max_collection_size << endl;
        log << "max time: " << max_time << endl;
        log << "wildcard plans: " << use_wildcard_plans << endl;
        log << "goal variables: ";
        for (const FactPair &goal : this->goals) {
            log << goal.var << ", ";
        }
        log << endl;
        log << "blacklisted variables: ";
        if (this->blacklisted_variables.empty()) {
            log << "none";
        } else {
            for (int var : this->blacklisted_variables) {
                log << var << ", ";
            }
        }
        log << endl;
    }

    utils::CountdownTimer timer(max_time);
    compute_initial_collection();
    int iteration = 1;
    State concrete_init = task_proxy.get_initial_state();
    concrete_init.unpack();
    int concrete_solution_index = -1;
    while (!time_limit_reached(timer)) {
        if (log.is_at_least_verbose()) {
            log << "iteration #" << iteration << endl;
        }

        FlawList flaws;
        concrete_solution_index = get_flaws(concrete_init, flaws);

        if (concrete_solution_index != -1) {
            if (log.is_at_least_normal()) {
                log << "task solved during computation of abstraction"
                    << endl;
            }
            break;
        }

        if (flaws.empty()) {
            if (log.is_at_least_normal()) {
                log << "flaw list empty. No further refinements possible"
                    << endl;
            }
            break;
        }

        refine(flaws);
        ++iteration;

        if (log.is_at_least_verbose()) {
            log << "current collection size: " << collection_size << endl;
            log << "current collection: ";
            print_collection();
            log << endl;
        }
    }
    if (log.is_at_least_verbose()) {
        log << endl;
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
        task_proxy, patterns, log);
    pattern_collection_information.set_pdbs(pdbs);

    if (log.is_at_least_normal()) {
        log << "CEGAR number of iterations: " << iteration << endl;
        dump_pattern_collection_generation_statistics(
            "CEGAR",
            timer.get_elapsed_time(),
            pattern_collection_information,
            log);
    }

    return pattern_collection_information;
}

PatternCollectionInformation generate_pattern_collection_with_cegar(
    int max_pdb_size,
    int max_collection_size,
    double max_time,
    bool use_wildcard_plans,
    utils::LogProxy &log,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const shared_ptr<AbstractTask> &task,
    const vector<FactPair> &goals,
    unordered_set<int> &&blacklisted_variables) {
    CEGAR cegar(
        max_pdb_size,
        max_collection_size,
        max_time,
        use_wildcard_plans,
        log,
        rng,
        task,
        goals,
        move(blacklisted_variables));
    return cegar.compute_pattern_collection();
}

PatternInformation generate_pattern_with_cegar(
    int max_pdb_size,
    double max_time,
    bool use_wildcard_plans,
    utils::LogProxy &log,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const shared_ptr<AbstractTask> &task,
    const FactPair &goal,
    unordered_set<int> &&blacklisted_variables) {
    vector<FactPair> goals = {goal};
    CEGAR cegar(
        max_pdb_size,
        max_pdb_size,
        max_time,
        use_wildcard_plans,
        log,
        rng,
        task,
        goals,
        move(blacklisted_variables));
    PatternCollectionInformation collection_info = cegar.compute_pattern_collection();
    shared_ptr<PatternCollection> new_patterns = collection_info.get_patterns();
    if (new_patterns->size() > 1) {
        cerr << "CEGAR limited to one goal computed more than one pattern" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }

    Pattern &pattern = new_patterns->front();
    shared_ptr<PDBCollection> new_pdbs = collection_info.get_pdbs();
    shared_ptr<PatternDatabase> &pdb = new_pdbs->front();
    PatternInformation result(TaskProxy(*task), move(pattern), log);
    result.set_pdb(pdb);
    return result;
}

void add_cegar_implementation_notes_to_parser(options::OptionParser &parser) {
    parser.document_note(
        "Short description of the CEGAR algorithm",
        "The CEGAR algorithm computes a pattern collection for a given planning "
        "task and a given (sub)set of its goals in a randomized order as "
        "follows. Starting from the pattern collection consisting of a singleton "
        "pattern for each goal variable, it repeatedly attempts to execute an "
        "optimal plan of each pattern in the concrete task, collects reasons why "
        "this is not possible (so-called flaws) and refines the pattern in "
        "question by adding a variable to it.\n"
        "Further parameters allow blacklisting a (sub)set of the non-goal "
        "variables which are then never added to the collection, limiting PDB "
        "and collection size, setting a time limit and switching between "
        "computing regular or wildcard plans, where the latter are sequences of "
        "parallel operators inducing the same abstract transition.",
        true);
    parser.document_note(
        "Implementation notes about the CEGAR algorithm",
        "The following describes differences of the implementation to "
        "the original implementation used and described in the paper.\n\n"
        "Conceptually, there is one larger difference which concerns the "
        "computation of (regular or wildcard) plans for PDBs. The original "
        "implementation used an enforced hill-climbing (EHC) search with the "
        "PDB as the perfect heuristic, which ensured finding strongly optimal "
        "plans, i.e., optimal plans with a minimum number of zero-cost "
        "operators, in domains with zero-cost operators. The original "
        "implementation also slightly modified EHC to search for a best-"
        "improving successor, chosen uniformly at random among all best-"
        "improving successors.\n\n"
        "In contrast, the current implementation computes a plan alongside the "
        "computation of the PDB itself. A modification to Dijkstra's algorithm "
        "for computing the PDB values stores, for each state, the operator "
        "leading to that state (in a regression search). This generating "
        "operator is updated only if the algorithm found a cheaper path to "
        "the state. After Dijkstra finishes, the plan computation starts at the "
        "initial state and iteratively follows the generating operator, computes "
        "all operators of the same cost inducing the same transition, until "
        "reaching a goal. This constitutes a wildcard plan. It is turned into a "
        "regular one by randomly picking a single operator for each transition. "
        "\n\n"
        "Note that this kind of plan extraction does not consider all successors "
        "of a state uniformly at random but rather uses the previously deterministically "
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
        "the description in the paper. The first bug fix is to raise a flaw "
        "for all goal variables of the task if the plan for a PDB can be "
        "executed on the concrete task but does not lead to a goal state. "
        "Previously, such flaws would not have been raised because all goal "
        "variables are part of the collection from the start on and therefore "
        "not considered. This means that the original implementation "
        "accidentally disallowed merging patterns due to goal violation "
        "flaws. The second bug fix is to actually randomize the order of "
        "parallel operators in wildcard plan steps.",
        true);
}

void add_cegar_wildcard_option_to_parser(options::OptionParser &parser) {
    parser.add_option<bool>(
        "use_wildcard_plans",
        "if true, compute wildcard plans which are sequences of sets of "
        "operators that induce the same transition; otherwise compute regular "
        "plans which are sequences of single operators",
        "true");
}
}
