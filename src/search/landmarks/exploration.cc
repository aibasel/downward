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
  Implementation note: Compared to RelaxationHeuristic, we *cannot simplify*
  unary operators, because this may conflict with excluded operators.
  For an example, consider that unary operator o1 is thrown out during
  simplify() because it is dominated by unary operator o2, but then o2
  is excluded during an exploration ==> the shared effect of o1 and o2 is
  wrongly never reached in the exploration.
*/

// Construction and destruction
Exploration::Exploration(const TaskProxy &task_proxy, utils::LogProxy &log)
    : task_proxy(task_proxy) {
    if (log.is_at_least_normal()) {
        log << "Initializing Exploration..." << endl;
    }

    // Build propositions.
    for (VariableProxy var : task_proxy.get_variables()) {
        int var_id = var.get_id();
        propositions.push_back(vector<Proposition>(var.get_domain_size()));
        for (int value = 0; value < var.get_domain_size(); ++value) {
            propositions[var_id][value].fact = FactPair(var_id, value);
        }
    }

    /*
      Reserve vector size unary operators. This is needed because we
      cross-reference to the memory address of elements of the vector while
      building it; meaning a resize would invalidate all references.
    */
    int num_unary_ops = 0;
    OperatorsProxy operators = task_proxy.get_operators();
    AxiomsProxy axioms = task_proxy.get_axioms();
    for (OperatorProxy op : operators) {
        num_unary_ops += op.get_effects().size();
    }
    for (OperatorProxy axiom : axioms) {
        num_unary_ops += axiom.get_effects().size();
    }
    unary_operators.reserve(num_unary_ops);

    // Build unary operators for operators and axioms.
    for (OperatorProxy op : operators)
        build_unary_operators(op);
    for (OperatorProxy axiom : axioms)
        build_unary_operators(axiom);
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
*/
void Exploration::setup_exploration_queue(
    const State &state, const vector<FactPair> &excluded_props,
    const vector<int> &excluded_op_ids) {
    prop_queue.clear();

    // Reset reachability information.
    for (auto &propositions_for_variable : propositions) {
        for (auto &prop : propositions_for_variable) {
            prop.reached = false;
        }
    }

    for (const FactPair &fact : excluded_props) {
        propositions[fact.var][fact.value].excluded = true;
    }

    // Set facts that are true in the current state as reached.
    for (FactProxy fact : state) {
        Proposition *init_prop =
            &propositions[fact.get_variable().get_id()][fact.get_value()];
        enqueue_if_necessary(init_prop);
    }

    /*
      Unary operators derived from operators that are excluded or achieve
      an excluded proposition *unconditionally* must be marked as excluded.

      Note that we in general cannot exclude all unary operators derived from
      operators that achieve an excluded propositon *conditionally*:
      Given an operator with uncoditional effect e1 and conditional effect e2
      with condition c yields unary operators uo1: {} -> e1 and uo2: c -> e2.
      Excluding both would not allow us to achieve e1 when excluding
      proposition e2. We instead only mark uo2 as excluded (see below when
      looping over all unary operators). Note however that this can lead to
      an overapproximation, e.g. if the effect e1 also has condition c.
    */
    unordered_set<int> op_ids_to_mark(excluded_op_ids.begin(),
                                      excluded_op_ids.end());
    for (OperatorProxy op : task_proxy.get_operators()) {
        for (EffectProxy effect : op.get_effects()) {
            if (effect.get_conditions().empty()
                && propositions[effect.get_fact().get_variable().get_id()]
                [effect.get_fact().get_value()].excluded) {
                op_ids_to_mark.insert(op.get_id());
                break;
            }
        }
    }

    // Initialize operator data, queue effects of precondition-free operators.
    for (UnaryOperator &op : unary_operators) {
        op.unsatisfied_preconditions = op.num_preconditions;

        /*
          Aside from UnaryOperators derived from operators with an id in
          op_ids_to_mark we also exclude UnaryOperators that have an excluded
          proposition as effect (see comment when building *op_ids_to_mark*).
        */
        if (op.effect->excluded
            || op_ids_to_mark.count(op.op_or_axiom_id)) {
            // Operator will not be applied during relaxed exploration.
            op.excluded = true;
            continue;
        }
        op.excluded = false; // Reset from previous exploration.

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
        for (UnaryOperator *unary_op : triggered_operators) {
            if (unary_op->excluded)
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
    const vector<int> &excluded_op_ids) {
    setup_exploration_queue(task_proxy.get_initial_state(),
                            excluded_props, excluded_op_ids);
    relaxed_exploration();

    // Bundle reachability information into the return data structure.
    vector<vector<bool>> reached;
    reached.resize(propositions.size());
    for (size_t var_id = 0; var_id < propositions.size(); ++var_id) {
        reached[var_id].resize(propositions[var_id].size(), false);
        for (size_t value = 0; value < propositions[var_id].size(); ++value) {
            if (propositions[var_id][value].reached) {
                reached[var_id][value] = true;
            }
        }
    }
    return reached;
}
}
