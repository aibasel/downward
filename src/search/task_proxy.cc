#include "task_proxy.h"

#include "axioms.h"

#include "task_utils/causal_graph.h"

#include <iostream>

using namespace std;

State State::get_successor(const OperatorProxy &op) const {
    assert(!op.is_axiom());
    //assert(is_applicable(op, state));
    vector<int> new_values = *values;
    for (EffectProxy effect : op.get_effects()) {
        if (does_fire(effect, *this)) {
            FactPair effect_fact = effect.get_fact().get_pair();
            new_values[effect_fact.var] = effect_fact.value;
        }
    }
    if (task->get_num_axioms() > 0) {
        AxiomEvaluator &axiom_evaluator = g_axiom_evaluators[TaskProxy(*task)];
        axiom_evaluator.evaluate(new_values);
    }
    return State(*task, move(new_values), StateHandle::unregistered_state);
}

const causal_graph::CausalGraph &TaskProxy::get_causal_graph() const {
    return causal_graph::get_causal_graph(task);
}
