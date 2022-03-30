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
/*
  Integration Note: this class is the same as (rich man's) FF heuristic
  (taken from hector branch) except for the following:
  - Added-on functionality for excluding certain operators from the relaxed
    exploration. (These operators are never applied, as necessary for landmark
    computation.)
  - Unary operators are not simplified, because this may conflict with excluded
    operators. (For an example, consider that unary operator o1 is thrown out
    during simplify() because it is dominated by unary operator o2, but then o2
    is excluded during an exploration ==> the shared effect of o1 and o2 is
    wrongly never reached in the exploration.)
*/

// Construction and destruction
Exploration::Exploration(const TaskProxy &task_proxy)
    : task_proxy(task_proxy) {
    utils::g_log << "Initializing Exploration..." << endl;

    // Build propositions.
    for (VariableProxy var : task_proxy.get_variables()) {
        int var_id = var.get_id();
        propositions.push_back(vector<Proposition>(var.get_domain_size()));
        for (int value = 0; value < var.get_domain_size(); ++value) {
            propositions[var_id][value].fact = FactPair(var_id, value);
        }
    }

    // Build unary operators for operators and axioms.
    OperatorsProxy operators = task_proxy.get_operators();
    for (OperatorProxy op : operators)
        build_unary_operators(op);
    AxiomsProxy axioms = task_proxy.get_axioms();
    for (OperatorProxy op : axioms)
        build_unary_operators(op);
}

void Exploration::build_unary_operators(const OperatorProxy &op) {
    // Note: changed from the original to allow sorting of operator conditions
    vector<Proposition *> precondition;
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
        Proposition *effect_proposition = &propositions[effect_fact.get_variable().get_id()][effect_fact.get_value()];
        int op_or_axiom_id = get_operator_or_axiom_id(op);
        unary_operators.emplace_back(precondition, effect_proposition, op_or_axiom_id);

        // Cross-reference unary operators.
        for (Proposition *pre : precondition) {
            pre->precondition_of.push_back(&unary_operators.back());
        }

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
            prop.reached = false;
        }
    }

    for (const FactPair &fact : excluded_props) {
        propositions[fact.var][fact.value].excluded = true;
    }

    // Deal with current state.
    for (FactProxy fact : state) {
        Proposition *init_prop =
            &propositions[fact.get_variable().get_id()][fact.get_value()];
        enqueue_if_necessary(init_prop);
    }

    // Initialize operator data, deal with precondition-free operators/axioms.
    for (UnaryOperator &op : unary_operators) {
        op.unsatisfied_preconditions = op.num_preconditions;
        if (op.effect->excluded
            || excluded_op_ids.count(op.op_or_axiom_id)) {
            // operator will not be applied during relaxed exploration
            op.excluded = true;
            continue;
        }
        op.excluded = false; // Reset from previous exploration

        if (op.unsatisfied_preconditions == 0) {
            enqueue_if_necessary(op.effect);
        }
    }

    // Reset *excluded* to false for the next exploration.
    for (const FactPair &fact : excluded_props) {
        propositions[fact.var][fact.value].excluded = false;
    }
}

void Exploration::relaxed_exploration() {
    while (!prop_queue.empty()) {
        Proposition *prop = prop_queue.front();
        prop_queue.pop_front();

        const vector<UnaryOperator *> &triggered_operators = prop->precondition_of;
        for (size_t i = 0; i < triggered_operators.size(); ++i) {
            UnaryOperator *unary_op = triggered_operators[i];
            if (unary_op->excluded) // operator is not applied
                continue;
            --unary_op->unsatisfied_preconditions;
            assert(unary_op->unsatisfied_preconditions >= 0);
            if (unary_op->unsatisfied_preconditions == 0) {
                enqueue_if_necessary(unary_op->effect);
            }
        }
    }
}

void Exploration::enqueue_if_necessary(Proposition *prop) {
    if (!prop->reached) {
        prop->reached = true;
        prop_queue.push_back(prop);
    }
}

vector<vector<bool>> Exploration::compute_relaxed_reachability(
    const vector<FactPair> &excluded_props,
    const unordered_set<int> &excluded_op_ids) {
    // set up reachability information
    vector<vector<bool>> reached;
    // TODO: it's maybe strange that we initialze reached through the
    // task proxy but fill it later on through propositions
    reached.reserve(task_proxy.get_variables().size());
    for (VariableProxy var : task_proxy.get_variables()) {
        reached.push_back(vector<bool>(var.get_domain_size(), false));
    }

    setup_exploration_queue(task_proxy.get_initial_state(), excluded_props, excluded_op_ids);
    relaxed_exploration();

    // Copy reachability information into reached.
    for (size_t var_id = 0; var_id < propositions.size(); ++var_id) {
        for (size_t value = 0; value < propositions[var_id].size(); ++value) {
            Proposition &prop = propositions[var_id][value];
            if (prop.reached)
                reached[var_id][value] = true;
        }
    }
    return reached;
}
}
