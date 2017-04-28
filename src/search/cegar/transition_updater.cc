#include "transition_updater.h"

#include "abstract_state.h"
#include "utils.h"

#include "../task_proxy.h"

#include "../task_utils/task_properties.h"

#include <algorithm>
#include <map>

using namespace std;

namespace cegar {
static vector<vector<FactPair>> get_preconditions_by_operator(
    const OperatorsProxy &ops) {
    vector<vector<FactPair>> preconditions_by_operator;
    preconditions_by_operator.reserve(ops.size());
    for (OperatorProxy op : ops) {
        vector<FactPair> preconditions = task_properties::get_fact_pairs(
            op.get_preconditions());
        sort(preconditions.begin(), preconditions.end());
        preconditions_by_operator.push_back(move(preconditions));
    }
    return preconditions_by_operator;
}

static vector<FactPair> get_postconditions(
    const OperatorProxy &op) {
    // Use map to obtain sorted postconditions.
    map<int, int> var_to_post;
    for (FactProxy fact : op.get_preconditions()) {
        var_to_post[fact.get_variable().get_id()] = fact.get_value();
    }
    for (EffectProxy effect : op.get_effects()) {
        FactPair fact = effect.get_fact().get_pair();
        var_to_post[fact.var] = fact.value;
    }
    vector<FactPair> postconditions;
    postconditions.reserve(var_to_post.size());
    for (const pair<int, int> &fact : var_to_post) {
        postconditions.emplace_back(fact.first, fact.second);
    }
    return postconditions;
}

static vector<vector<FactPair>> get_postconditions_by_operator(
    const OperatorsProxy &ops) {
    vector<vector<FactPair>> postconditions_by_operator;
    postconditions_by_operator.reserve(ops.size());
    for (OperatorProxy op : ops) {
        postconditions_by_operator.push_back(get_postconditions(op));
    }
    return postconditions_by_operator;
}

static int lookup_value(const vector<FactPair> &facts, int var) {
    assert(is_sorted(facts.begin(), facts.end()));
    for (const FactPair &fact : facts) {
        if (fact.var == var) {
            return fact.value;
        } else if (fact.var > var) {
            return UNDEFINED_VALUE;
        }
    }
    return UNDEFINED_VALUE;
}


TransitionUpdater::TransitionUpdater(const OperatorsProxy &ops)
    : preconditions_by_operator(get_preconditions_by_operator(ops)),
      postconditions_by_operator(get_postconditions_by_operator(ops)),
      num_non_loops(0),
      num_loops(0) {
}

int TransitionUpdater::get_precondition_value(int op_id, int var) const {
    return lookup_value(preconditions_by_operator[op_id], var);
}

int TransitionUpdater::get_postcondition_value(int op_id, int var) const {
    return lookup_value(postconditions_by_operator[op_id], var);
}

void TransitionUpdater::add_loops_to_trivial_abstract_state(AbstractState *state) {
    for (size_t i = 0; i < preconditions_by_operator.size(); ++i) {
        add_loop(state, i);
    }
}

void TransitionUpdater::add_transition(
    AbstractState *src, int op_id, AbstractState *target) {
    assert(src != target);
    src->add_outgoing_transition(op_id, target);
    target->add_incoming_transition(op_id, src);
    ++num_non_loops;
}

void TransitionUpdater::add_loop(AbstractState *state, int op_id) {
    state->add_loop(op_id);
    ++num_loops;
}

void TransitionUpdater::remove_incoming_transition(
    AbstractState *src, int op_id, AbstractState *target) {
    target->remove_incoming_transition(op_id, src);
    --num_non_loops;
}

void TransitionUpdater::remove_outgoing_transition(
    AbstractState *src, int op_id, AbstractState *target) {
    src->remove_outgoing_transition(op_id, target);
    --num_non_loops;
}

void TransitionUpdater::rewire_incoming_transitions(
    AbstractState *v, AbstractState *v1, AbstractState *v2, int var) {
    /* State v has been split into v1 and v2. Now for all transitions
       u->v we need to add transitions u->v1, u->v2, or both. */
    for (const Transition &transition : v->get_incoming_transitions()) {
        int op_id = transition.op_id;
        AbstractState *u = transition.target;
        assert(u != v);
        int post = get_postcondition_value(op_id, var);
        if (post == UNDEFINED_VALUE) {
            // op has no precondition and no effect on var.
            bool u_and_v1_intersect = u->domains_intersect(v1, var);
            if (u_and_v1_intersect) {
                add_transition(u, op_id, v1);
            }
            /* If the domains of u and v1 don't intersect, we must add
               the other transition and can avoid an intersection test. */
            if (!u_and_v1_intersect || u->domains_intersect(v2, var)) {
                add_transition(u, op_id, v2);
            }
        } else if (v1->contains(var, post)) {
            // op can only end in v1.
            add_transition(u, op_id, v1);
        } else {
            // op can only end in v2.
            assert(v2->contains(var, post));
            add_transition(u, op_id, v2);
        }
        remove_outgoing_transition(u, op_id, v);
    }
}

void TransitionUpdater::rewire_outgoing_transitions(
    AbstractState *v, AbstractState *v1, AbstractState *v2, int var) {
    /* State v has been split into v1 and v2. Now for all transitions
       v->w we need to add transitions v1->w, v2->w, or both. */
    for (const Transition &transition : v->get_outgoing_transitions()) {
        int op_id = transition.op_id;
        AbstractState *w = transition.target;
        assert(w != v);
        int pre = get_precondition_value(op_id, var);
        int post = get_postcondition_value(op_id, var);
        if (post == UNDEFINED_VALUE) {
            assert(pre == UNDEFINED_VALUE);
            // op has no precondition and no effect on var.
            bool v1_and_w_intersect = v1->domains_intersect(w, var);
            if (v1_and_w_intersect) {
                add_transition(v1, op_id, w);
            }
            /* If the domains of v1 and w don't intersect, we must add
               the other transition and can avoid an intersection test. */
            if (!v1_and_w_intersect || v2->domains_intersect(w, var)) {
                add_transition(v2, op_id, w);
            }
        } else if (pre == UNDEFINED_VALUE) {
            // op has no precondition, but an effect on var.
            add_transition(v1, op_id, w);
            add_transition(v2, op_id, w);
        } else if (v1->contains(var, pre)) {
            // op can only start in v1.
            add_transition(v1, op_id, w);
        } else {
            // op can only start in v2.
            assert(v2->contains(var, pre));
            add_transition(v2, op_id, w);
        }
        remove_incoming_transition(v, op_id, w);
    }
}

void TransitionUpdater::rewire_loops(
    AbstractState *v, AbstractState *v1, AbstractState *v2, int var) {
    /* State v has been split into v1 and v2. Now for all self-loops
       v->v we need to add one or two of the transitions v1->v1, v1->v2,
       v2->v1 and v2->v2. */
    for (int op_id : v->get_loops()) {
        int pre = get_precondition_value(op_id, var);
        int post = get_postcondition_value(op_id, var);
        if (pre == UNDEFINED_VALUE) {
            // op has no precondition on var --> it must start in v1 and v2.
            if (post == UNDEFINED_VALUE) {
                // op has no effect on var --> it must end in v1 and v2.
                add_loop(v1, op_id);
                add_loop(v2, op_id);
            } else if (v2->contains(var, post)) {
                // op must end in v2.
                add_transition(v1, op_id, v2);
                add_loop(v2, op_id);
            } else {
                // op must end in v1.
                assert(v1->contains(var, post));
                add_loop(v1, op_id);
                add_transition(v2, op_id, v1);
            }
        } else if (v1->contains(var, pre)) {
            // op must start in v1.
            assert(post != UNDEFINED_VALUE);
            if (v1->contains(var, post)) {
                // op must end in v1.
                add_loop(v1, op_id);
            } else {
                // op must end in v2.
                assert(v2->contains(var, post));
                add_transition(v1, op_id, v2);
            }
        } else {
            // op must start in v2.
            assert(v2->contains(var, pre));
            assert(post != UNDEFINED_VALUE);
            if (v1->contains(var, post)) {
                // op must end in v1.
                add_transition(v2, op_id, v1);
            } else {
                // op must end in v2.
                assert(v2->contains(var, post));
                add_loop(v2, op_id);
            }
        }
    }
    num_loops -= v->get_loops().size();
}

void TransitionUpdater::rewire(
    AbstractState *v, AbstractState *v1, AbstractState *v2, int var) {
    rewire_incoming_transitions(v, v1, v2, var);
    rewire_outgoing_transitions(v, v1, v2, var);
    rewire_loops(v, v1, v2, var);
}

int TransitionUpdater::get_num_non_loops() const {
    return num_non_loops;
}

int TransitionUpdater::get_num_loops() const {
    return num_loops;
}
}
