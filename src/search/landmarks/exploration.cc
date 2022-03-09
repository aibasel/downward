#include "exploration.h"

#include "util.h"

#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/hash.h"
#include "../utils/logging.h"

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
    : task_proxy(task_proxy) {
    utils::g_log << "Initializing Exploration..." << endl;

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

/*
  This function initializes the priority queue and the information associated
  with propositions and unary operators for the relaxed exploration. Unary
  operators that are not allowed to be applied due to exclusions are marked by
  setting their *excluded* flag.

  *excluded_op_ids* should contain at least the operators that achieve an
  excluded proposition unconditionally. There are two contexts where
  *compute_reachability_with_excludes()* (and hence this function) is used:
  (a) in *LandmarkFactoryRelaxation::relaxed_task_solvable()* where indeed
      all operators are excluded if they achieve an excluded proposition
      unconditionally; and
  (b) in *LandmarkFactoryRelaxation::is_causal_landmark()* where
      *excluded_props* is empty and hence no operators achieve an excluded
      proposition. (*excluded_op_ids* "additionally" contains operators that
      have a landmark as their precondition in this context.)

  TODO: issue1045 aims at moving the logic to exclude operators that achieve
   excluded propositions here to consistently do so in all contexts.
*/
void Exploration::setup_exploration_queue(
    const State &state, const vector<FactPair> &excluded_props,
    const unordered_set<int> &excluded_op_ids) {
    prop_queue.clear();

    for (auto &propositions_for_variable : propositions) {
        for (auto &prop : propositions_for_variable) {
            prop.h_max_cost = -1;
        }
    }

    for (const FactPair &fact : excluded_props) {
        propositions[fact.var][fact.value].excluded = true;
    }

    // Deal with current state.
    for (FactProxy fact : state) {
        ExProposition *init_prop =
            &propositions[fact.get_variable().get_id()][fact.get_value()];
        enqueue_if_necessary(init_prop, 0);
    }

    // Initialize operator data, deal with precondition-free operators/axioms.
    for (ExUnaryOperator &op : unary_operators) {
        op.unsatisfied_preconditions = op.precondition.size();
        if (op.effect->excluded
            || excluded_op_ids.count(op.op_or_axiom_id)) {
            // operator will not be applied during relaxed exploration
            op.excluded = true;
            continue;
        }
        op.excluded = false; // Reset from previous exploration
        op.h_max_cost = op.base_cost;

        if (op.unsatisfied_preconditions == 0) {
            enqueue_if_necessary(op.effect, op.base_cost);
        }
    }

    // Reset *excluded* to false for the next exploration.
    for (const FactPair &fact : excluded_props) {
        propositions[fact.var][fact.value].excluded = false;
    }
}

void Exploration::relaxed_exploration() {
    while (!prop_queue.empty()) {
        pair<int, ExProposition *> top_pair = prop_queue.pop();
        int distance = top_pair.first;
        ExProposition *prop = top_pair.second;

        int prop_cost = prop->h_max_cost;
        assert(prop_cost <= distance);
        if (prop_cost < distance)
            continue;
        const vector<ExUnaryOperator *> &triggered_operators = prop->precondition_of;
        for (size_t i = 0; i < triggered_operators.size(); ++i) {
            ExUnaryOperator *unary_op = triggered_operators[i];
            if (unary_op->excluded) // operator is not applied
                continue;
            --unary_op->unsatisfied_preconditions;
            unary_op->h_max_cost = max(prop_cost + unary_op->base_cost,
                                       unary_op->h_max_cost);
            assert(unary_op->unsatisfied_preconditions >= 0);
            if (unary_op->unsatisfied_preconditions == 0) {
                enqueue_if_necessary(unary_op->effect, unary_op->h_max_cost);
            }
        }
    }
}

void Exploration::enqueue_if_necessary(ExProposition *prop, int cost) {
    assert(cost >= 0);
    if (prop->h_max_cost == -1 || prop->h_max_cost > cost) {
        prop->h_max_cost = cost;
        prop_queue.push(cost, prop);
    }
}

void Exploration::compute_reachability_with_excludes(
    vector<vector<int>> &lvl_var,
    const vector<FactPair> &excluded_props,
    const unordered_set<int> &excluded_op_ids) {
    // Perform exploration using h_max-values.
    setup_exploration_queue(task_proxy.get_initial_state(), excluded_props, excluded_op_ids);
    relaxed_exploration();

    // Copy reachability information into lvl_var.
    for (size_t var_id = 0; var_id < propositions.size(); ++var_id) {
        for (size_t value = 0; value < propositions[var_id].size(); ++value) {
            ExProposition &prop = propositions[var_id][value];
            if (prop.h_max_cost >= 0)
                lvl_var[var_id][value] = prop.h_max_cost;
        }
    }
}
}
