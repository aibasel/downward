#include "exploration.h"

#include "util.h"

#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/hash.h"

#include <algorithm>
#include <cassert>
#include <limits>

using namespace std;

namespace landmarks {
/* Integration Note: this class is the same as (rich man's) FF heuristic
   (taken from hector branch) except for the following:
   - Added-on functionality for excluding certain operators from the relaxed
     exploration (these operators are never applied, as necessary for landmark
     computation)
   - Exploration can be done either using h_max or h_add criterion (h_max is needed
     during landmark generation, h_add later for heuristic computations)
   - Unary operators are not simplified, because this may conflict with excluded
     operators. (For an example, consider that unary operator o1 is thrown out
     during simplify() because it is dominated by unary operator o2, but then o2
     is excluded during an exploration ==> the shared effect of o1 and o2 is wrongly
     never reached in the exploration.)
   - Added-on functionality to check for a set of propositions which one is reached
     first by the relaxed exploration, and extract preferred operators for this cheapest
     proposition (needed for planning to nearest landmark).
*/

// Construction and destruction
Exploration::Exploration(const TaskProxy &task_proxy)
    : task_proxy(task_proxy),
      did_write_overflow_warning(false) {
    cout << "Initializing Exploration..." << endl;

    // Build propositions.
    for (VariableProxy var : task_proxy.get_variables()) {
        int var_id = var.get_id();
        propositions.push_back(vector<ExProposition>(var.get_domain_size()));
        for (int value = 0; value < var.get_domain_size(); ++value) {
            propositions[var_id][value].fact = FactPair(var_id, value);
        }
    }

    // Build goal propositions.
    for (FactProxy goal_fact : task_proxy.get_goals()) {
        int var_id = goal_fact.get_variable().get_id();
        int value = goal_fact.get_value();
        propositions[var_id][value].is_goal_condition = true;
        propositions[var_id][value].is_termination_condition = true;
        goal_propositions.push_back(&propositions[var_id][value]);
        termination_propositions.push_back(&propositions[var_id][value]);
    }

    // Build unary operators for operators and axioms.
    OperatorsProxy operators = task_proxy.get_operators();
    for (OperatorProxy op : operators)
        build_unary_operators(op);
    AxiomsProxy axioms = task_proxy.get_axioms();
    for (OperatorProxy op : axioms)
        build_unary_operators(op);

    // Cross-reference unary operators.
    for (ExUnaryOperator &op : unary_operators) {
        for (ExProposition *pre : op.precondition)
            pre->precondition_of.push_back(&op);
    }
}

void Exploration::increase_cost(int &cost, int amount) {
    assert(cost >= 0);
    assert(amount >= 0);
    cost += amount;
    if (cost > MAX_COST_VALUE) {
        write_overflow_warning();
        cost = MAX_COST_VALUE;
    }
}

void Exploration::write_overflow_warning() {
    if (!did_write_overflow_warning) {
        // TODO: Should have a planner-wide warning mechanism to handle
        // things like this.
        cout << "WARNING: overflow on landmark exploration h^add! Costs clamped to "
             << MAX_COST_VALUE << endl;
        did_write_overflow_warning = true;
    }
}

void Exploration::build_unary_operators(const OperatorProxy &op) {
    // Note: changed from the original to allow sorting of operator conditions
    int base_cost = op.get_cost();
    vector<ExProposition *> precondition;
    vector<FactPair> precondition_facts1;

    for (FactProxy pre : op.get_preconditions()) {
        precondition_facts1.push_back(pre.get_pair());
    }
    for (EffectProxy effect : op.get_effects()) {
        vector<FactPair> precondition_facts2(precondition_facts1);
        EffectConditionsProxy effect_conditions = effect.get_conditions();
        for (FactProxy effect_condition : effect_conditions) {
            precondition_facts2.push_back(effect_condition.get_pair());
        }

        sort(precondition_facts2.begin(), precondition_facts2.end());

        for (const FactPair &precondition_fact : precondition_facts2)
            precondition.push_back(&propositions[precondition_fact.var]
                                   [precondition_fact.value]);

        FactProxy effect_fact = effect.get_fact();
        ExProposition *effect_proposition = &propositions[effect_fact.get_variable().get_id()][effect_fact.get_value()];
        int op_or_axiom_id = get_operator_or_axiom_id(op);
        unary_operators.emplace_back(precondition, effect_proposition, op_or_axiom_id, base_cost);
        precondition.clear();
        precondition_facts2.clear();
    }
}

// heuristic computation
void Exploration::setup_exploration_queue(const State &state,
                                          const vector<FactPair> &excluded_props,
                                          const unordered_set<int> &excluded_op_ids,
                                          bool use_h_max) {
    prop_queue.clear();

    for (size_t var_id = 0; var_id < propositions.size(); ++var_id) {
        for (size_t value = 0; value < propositions[var_id].size(); ++value) {
            ExProposition &prop = propositions[var_id][value];
            prop.h_add_cost = -1;
            prop.h_max_cost = -1;
            prop.depth = -1;
            prop.marked = false;
        }
    }

    for (const FactPair &fact : excluded_props) {
        ExProposition &prop = propositions[fact.var][fact.value];
        prop.h_add_cost = -2;
    }

    // Deal with current state.
    for (FactProxy fact : state) {
        ExProposition *init_prop = &propositions[fact.get_variable().get_id()][fact.get_value()];
        enqueue_if_necessary(init_prop, 0, 0, 0, use_h_max);
    }

    // Initialize operator data, deal with precondition-free operators/axioms.
    for (ExUnaryOperator &op : unary_operators) {
        op.unsatisfied_preconditions = op.precondition.size();
        if (!excluded_op_ids.empty() &&
            (op.effect->h_add_cost == -2 || excluded_op_ids.count(op.op_or_axiom_id))) {
            op.h_add_cost = -2; // operator will not be applied during relaxed exploration
            continue;
        }
        op.h_add_cost = op.base_cost; // will be increased by precondition costs
        op.h_max_cost = op.base_cost;
        op.depth = -1;

        if (op.unsatisfied_preconditions == 0) {
            op.depth = 0;
            int depth = op.is_induced_by_axiom(task_proxy) ? 0 : 1;
            enqueue_if_necessary(op.effect, op.base_cost, depth, &op, use_h_max);
        }
    }
}

void Exploration::relaxed_exploration(bool use_h_max, bool level_out) {
    int unsolved_goals = termination_propositions.size();
    while (!prop_queue.empty()) {
        pair<int, ExProposition *> top_pair = prop_queue.pop();
        int distance = top_pair.first;
        ExProposition *prop = top_pair.second;

        int prop_cost;
        if (use_h_max)
            prop_cost = prop->h_max_cost;
        else
            prop_cost = prop->h_add_cost;
        assert(prop_cost <= distance);
        if (prop_cost < distance)
            continue;
        if (!level_out && prop->is_termination_condition && --unsolved_goals == 0)
            return;
        const vector<ExUnaryOperator *> &triggered_operators = prop->precondition_of;
        for (size_t i = 0; i < triggered_operators.size(); ++i) {
            ExUnaryOperator *unary_op = triggered_operators[i];
            if (unary_op->h_add_cost == -2) // operator is not applied
                continue;
            --unary_op->unsatisfied_preconditions;
            increase_cost(unary_op->h_add_cost, prop_cost);
            unary_op->h_max_cost = max(prop_cost + unary_op->base_cost,
                                       unary_op->h_max_cost);
            unary_op->depth = max(unary_op->depth, prop->depth);
            assert(unary_op->unsatisfied_preconditions >= 0);
            if (unary_op->unsatisfied_preconditions == 0) {
                int depth = unary_op->is_induced_by_axiom(task_proxy)
                    ? unary_op->depth : unary_op->depth + 1;
                if (use_h_max)
                    enqueue_if_necessary(unary_op->effect, unary_op->h_max_cost,
                                         depth, unary_op, use_h_max);
                else
                    enqueue_if_necessary(unary_op->effect, unary_op->h_add_cost,
                                         depth, unary_op, use_h_max);
            }
        }
    }
}

void Exploration::enqueue_if_necessary(ExProposition *prop, int cost, int depth,
                                       ExUnaryOperator *op,
                                       bool use_h_max) {
    assert(cost >= 0);
    if (use_h_max && (prop->h_max_cost == -1 || prop->h_max_cost > cost)) {
        prop->h_max_cost = cost;
        prop->depth = depth;
        prop->reached_by = op;
        prop_queue.push(cost, prop);
    } else if (!use_h_max && (prop->h_add_cost == -1 || prop->h_add_cost > cost)) {
        prop->h_add_cost = cost;
        prop->depth = depth;
        prop->reached_by = op;
        prop_queue.push(cost, prop);
    }
    if (use_h_max)
        assert(prop->h_max_cost != -1 &&
               prop->h_max_cost <= cost);
    else
        assert(prop->h_add_cost != -1 &&
               prop->h_add_cost <= cost);
}

void Exploration::compute_reachability_with_excludes(vector<vector<int>> &lvl_var,
                                                     vector<utils::HashMap<FactPair, int>> &lvl_op,
                                                     bool level_out,
                                                     const vector<FactPair> &excluded_props,
                                                     const unordered_set<int> &excluded_op_ids,
                                                     bool compute_lvl_ops) {
    // Perform exploration using h_max-values
    setup_exploration_queue(task_proxy.get_initial_state(), excluded_props, excluded_op_ids, true);
    relaxed_exploration(true, level_out);

    // Copy reachability information into lvl_var and lvl_op
    for (size_t var_id = 0; var_id < propositions.size(); ++var_id) {
        for (size_t value = 0; value < propositions[var_id].size(); ++value) {
            ExProposition &prop = propositions[var_id][value];
            if (prop.h_max_cost >= 0)
                lvl_var[var_id][value] = prop.h_max_cost;
        }
    }
    if (compute_lvl_ops) {
        for (ExUnaryOperator &op : unary_operators) {
            // H_max_cost of operator might be wrongly 0 or 1, if the operator
            // did not get applied during relaxed exploration. Look through
            // preconditions and adjust.
            for (ExProposition *prop : op.precondition) {
                if (prop->h_max_cost == -1) {
                    // Operator cannot be applied due to unreached precondition
                    op.h_max_cost = numeric_limits<int>::max();
                    break;
                } else if (op.h_max_cost < prop->h_max_cost + op.base_cost)
                    op.h_max_cost = prop->h_max_cost + op.base_cost;
            }
            if (op.h_max_cost == numeric_limits<int>::max())
                break;
            // We subtract 1 to keep semantics for landmark code:
            // if op can achieve prop at time step i+1,
            // its index (for prop) is i, where the initial state is time step 0.
            const FactPair &effect = op.effect->fact;
            assert(lvl_op[op.op_or_axiom_id].count(effect));
            int new_lvl = op.h_max_cost - 1;
            // If we have found a cheaper achieving operator, adjust h_max cost of proposition.
            if (lvl_op[op.op_or_axiom_id].find(effect)->second > new_lvl)
                lvl_op[op.op_or_axiom_id].find(effect)->second = new_lvl;
        }
    }
}
}
