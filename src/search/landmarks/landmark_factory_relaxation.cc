#include "landmark_factory_relaxation.h"

#include "exploration.h"

using namespace std;

namespace landmarks {
void LandmarkFactoryRelaxation::generate_landmarks(const shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    Exploration exploration(task_proxy);
    generate_relaxed_landmarks(task, exploration);
    generate(task_proxy, exploration);
}

void LandmarkFactoryRelaxation::generate(const TaskProxy &task_proxy, Exploration &exploration) {
    if (only_causal_landmarks)
        discard_noncausal_landmarks(task_proxy, exploration);
    if (!disjunctive_landmarks)
        discard_disjunctive_landmarks();
    if (!conjunctive_landmarks)
        discard_conjunctive_landmarks();
    lm_graph->set_landmark_ids();

    if (no_orders)
        discard_all_orderings();
    else if (reasonable_orders) {
        utils::g_log << "approx. reasonable orders" << endl;
        approximate_reasonable_orders(task_proxy, false);
        utils::g_log << "approx. obedient reasonable orders" << endl;
        approximate_reasonable_orders(task_proxy, true);
    }
    mk_acyclic_graph();
    calc_achievers(task_proxy, exploration);
}

void LandmarkFactoryRelaxation::discard_noncausal_landmarks(
    const TaskProxy &task_proxy, Exploration &exploration) {
    int num_all_landmarks = lm_graph->get_num_landmarks();
    lm_graph->remove_node_if(
        [this, &task_proxy, &exploration](const LandmarkNode &node) {
            return !is_causal_landmark(task_proxy, exploration, node);
        });
    int num_causal_landmarks = lm_graph->get_num_landmarks();
    utils::g_log << "Discarded " << num_all_landmarks - num_causal_landmarks
                 << " non-causal landmarks" << endl;
}

bool LandmarkFactoryRelaxation::is_causal_landmark(
    const TaskProxy &task_proxy, Exploration &exploration,
    const LandmarkNode &landmark) const {
    /* Test whether the relaxed planning task is unsolvable without using any operator
       that has "landmark" as a precondition.
       Similar to "relaxed_task_solvable" above.
     */

    if (landmark.is_true_in_goal)
        return true;
    vector<vector<int>> lvl_var;
    vector<utils::HashMap<FactPair, int>> lvl_op;
    // Initialize lvl_var to numeric_limits<int>::max()
    VariablesProxy variables = task_proxy.get_variables();
    lvl_var.resize(variables.size());
    for (VariableProxy var : variables) {
        lvl_var[var.get_id()].resize(var.get_domain_size(),
                                     numeric_limits<int>::max());
    }
    unordered_set<int> exclude_op_ids;
    vector<FactPair> exclude_props;
    for (OperatorProxy op : task_proxy.get_operators()) {
        if (is_landmark_precondition(op, &landmark)) {
            exclude_op_ids.insert(op.get_id());
        }
    }
    // Do relaxed exploration
    exploration.compute_reachability_with_excludes(
        lvl_var, lvl_op, true, exclude_props, exclude_op_ids, false);

    // Test whether all goal propositions have a level of less than numeric_limits<int>::max()
    for (FactProxy goal : task_proxy.get_goals())
        if (lvl_var[goal.get_variable().get_id()][goal.get_value()] ==
            numeric_limits<int>::max())
            return true;

    return false;
}

void LandmarkFactoryRelaxation::calc_achievers(const TaskProxy &task_proxy, Exploration &exploration) {
    VariablesProxy variables = task_proxy.get_variables();
    for (auto &lmn : lm_graph->get_nodes()) {
        for (const FactPair &lm_fact : lmn->facts) {
            const vector<int> &ops = get_operators_including_eff(lm_fact);
            lmn->possible_achievers.insert(ops.begin(), ops.end());

            if (variables[lm_fact.var].is_derived())
                lmn->is_derived = true;
        }

        vector<vector<int>> lvl_var;
        vector<utils::HashMap<FactPair, int>> lvl_op;
        relaxed_task_solvable(task_proxy, exploration, lvl_var, lvl_op, true, lmn.get());

        for (int op_or_axom_id : lmn->possible_achievers) {
            OperatorProxy op = get_operator_or_axiom(task_proxy, op_or_axom_id);

            if (_possibly_reaches_lm(op, lvl_var, lmn.get())) {
                lmn->first_achievers.insert(op_or_axom_id);
            }
        }
    }
}

bool LandmarkFactoryRelaxation::relaxed_task_solvable(
    const TaskProxy &task_proxy, Exploration &exploration, bool level_out,
    const LandmarkNode *exclude, bool compute_lvl_op) const {
    vector<vector<int>> lvl_var;
    vector<utils::HashMap<FactPair, int>> lvl_op;
    return relaxed_task_solvable(task_proxy, exploration, lvl_var, lvl_op, level_out, exclude, compute_lvl_op);
}

bool LandmarkFactoryRelaxation::relaxed_task_solvable(
    const TaskProxy &task_proxy, Exploration &exploration,
    vector<vector<int>> &lvl_var, vector<utils::HashMap<FactPair, int>> &lvl_op,
    bool level_out, const LandmarkNode *exclude, bool compute_lvl_op) const {
    /* Test whether the relaxed planning task is solvable without achieving the propositions in
     "exclude" (do not apply operators that would add a proposition from "exclude").
     As a side effect, collect in lvl_var and lvl_op the earliest possible point in time
     when a proposition / operator can be achieved / become applicable in the relaxed task.
     */

    OperatorsProxy operators = task_proxy.get_operators();
    AxiomsProxy axioms = task_proxy.get_axioms();
    // Initialize lvl_op and lvl_var to numeric_limits<int>::max()
    if (compute_lvl_op) {
        lvl_op.resize(operators.size() + axioms.size());
        for (OperatorProxy op : operators) {
            add_operator_and_propositions_to_list(op, lvl_op);
        }
        for (OperatorProxy axiom : axioms) {
            add_operator_and_propositions_to_list(axiom, lvl_op);
        }
    }
    VariablesProxy variables = task_proxy.get_variables();
    lvl_var.resize(variables.size());
    for (VariableProxy var : variables) {
        lvl_var[var.get_id()].resize(var.get_domain_size(),
                                     numeric_limits<int>::max());
    }
    // Extract propositions from "exclude"
    unordered_set<int> exclude_op_ids;
    vector<FactPair> exclude_props;
    if (exclude) {
        for (OperatorProxy op : operators) {
            if (achieves_non_conditional(op, exclude))
                exclude_op_ids.insert(op.get_id());
        }
        exclude_props.insert(exclude_props.end(),
                             exclude->facts.begin(), exclude->facts.end());
    }
    // Do relaxed exploration
    exploration.compute_reachability_with_excludes(
        lvl_var, lvl_op, level_out, exclude_props, exclude_op_ids, compute_lvl_op);

    // Test whether all goal propositions have a level of less than numeric_limits<int>::max()
    for (FactProxy goal : task_proxy.get_goals())
        if (lvl_var[goal.get_variable().get_id()][goal.get_value()] ==
            numeric_limits<int>::max())
            return false;

    return true;
}

bool LandmarkFactoryRelaxation::achieves_non_conditional(
    const OperatorProxy &o, const LandmarkNode *lmp) const {
    /* Test whether the landmark is achieved by the operator unconditionally.
    A disjunctive landmark is achieved if one of its disjuncts is achieved. */
    assert(lmp);
    for (EffectProxy effect: o.get_effects()) {
        for (const FactPair &lm_fact : lmp->facts) {
            FactProxy effect_fact = effect.get_fact();
            if (effect_fact.get_pair() == lm_fact) {
                if (effect.get_conditions().empty())
                    return true;
            }
        }
    }
    return false;
}

void LandmarkFactoryRelaxation::add_operator_and_propositions_to_list(
    const OperatorProxy &op, vector<utils::HashMap<FactPair, int>> &lvl_op) const {
    int op_or_axiom_id = get_operator_or_axiom_id(op);
    for (EffectProxy effect : op.get_effects()) {
        lvl_op[op_or_axiom_id].emplace(effect.get_fact().get_pair(), numeric_limits<int>::max());
    }
}
}
