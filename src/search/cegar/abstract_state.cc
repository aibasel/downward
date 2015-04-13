#include "abstract_state.h"

#include "split_tree.h"
#include "utils.h"

#include "../task_proxy.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <unordered_set>

using namespace std;

namespace cegar {
AbstractState::AbstractState(const Values &values, Node *node)
    : values(values),
      node(node),
      distance(UNDEFINED) {
}

string AbstractState::str() const {
    ostringstream oss;
    oss << "<" << values.str() << ">";
    return oss.str();
}

size_t AbstractState::count(int var) const {
    return values.count(var);
}

Values AbstractState::regress(OperatorProxy op) const {
    Values regressed_values = values;
    unordered_set<int> precondition_vars;
    for (FactProxy precondition : op.get_preconditions()) {
        int var_id = precondition.get_variable().get_id();
        regressed_values.set(var_id, precondition.get_value());
        precondition_vars.insert(var_id);
    }
    for (EffectProxy effect : op.get_effects()) {
        int var_id = effect.get_fact().get_variable().get_id();
        if (precondition_vars.count(var_id) == 0) {
            regressed_values.add_all(var_id);
        }
    }
    return regressed_values;
}

void AbstractState::get_possible_flaws(const AbstractState &desired,
                                        const State &prev_conc_state,
                                        Flaws *flaws)
const {
    values.get_possible_flaws(desired.values, prev_conc_state, flaws);
}

bool AbstractState::domains_intersect(const AbstractState *other, int var) const {
    return values.domains_intersect(other->values, var);
}

void AbstractState::update_incoming_arcs(int var, AbstractState *v1, AbstractState *v2) {
    for (auto arc : arcs_in) {
        OperatorProxy op = arc.first;
        AbstractState *u = arc.second;
        assert(u != this);
        int post = get_post(op, var);
        if (post == UNDEFINED) {
            // If the domains of u and v1 don't intersect, we must add the other arc.
            bool u_and_v1_intersect = u->domains_intersect(v1, var);
            if (u_and_v1_intersect) {
                u->add_arc(op, v1);
            }
            if (!u_and_v1_intersect || u->domains_intersect(v2, var)) {
                u->add_arc(op, v2);
            }
        } else if (v2->values.test(var, post)) {
            u->add_arc(op, v2);
        } else {
            u->add_arc(op, v1);
        }
        u->remove_next_arc(op, this);
    }
}

void AbstractState::update_outgoing_arcs(int var, AbstractState *v1, AbstractState *v2) {
    for (Arcs::iterator it = arcs_out.begin(); it != arcs_out.end(); ++it) {
        OperatorProxy op = it->first;
        AbstractState *w = it->second;
        assert(w != this);
        int pre = get_pre(op, var);
        int post = get_post(op, var);
        if (post == UNDEFINED) {
            assert(pre == UNDEFINED);
            // If the domains of v1 and w don't intersect, we must add the other arc.
            bool v1_and_w_intersect = v1->domains_intersect(w, var);
            if (v1_and_w_intersect) {
                v1->add_arc(op, w);
            }
            if (!v1_and_w_intersect || v2->domains_intersect(w, var)) {
                v2->add_arc(op, w);
            }
        } else if (pre == UNDEFINED) {
            v1->add_arc(op, w);
            v2->add_arc(op, w);
        } else if (v2->values.test(var, pre)) {
            v2->add_arc(op, w);
        } else {
            v1->add_arc(op, w);
        }
        w->remove_prev_arc(op, this);
    }
}

void AbstractState::update_loops(int var, AbstractState *v1, AbstractState *v2) {
    for (OperatorProxy op : loops) {
        int pre = get_pre(op, var);
        int post = get_post(op, var);
        if (pre == UNDEFINED) {
            if (post == UNDEFINED) {
                v1->add_loop(op);
                v2->add_loop(op);
            } else if (v2->values.test(var, post)) {
                v1->add_arc(op, v2);
                v2->add_loop(op);
            } else {
                assert(v1->values.test(var, post));
                v1->add_loop(op);
                v2->add_arc(op, v1);
            }
        } else if (v2->values.test(var, pre)) {
            assert(post != UNDEFINED);
            if (v2->values.test(var, post)) {
                v2->add_loop(op);
            } else {
                assert(v1->values.test(var, post));
                v2->add_arc(op, v1);
            }
        } else if (v2->values.test(var, post)) {
            v1->add_arc(op, v2);
        } else {
            assert(v1->values.test(var, post));
            v1->add_loop(op);
        }
    }
}

pair<AbstractState *, AbstractState *> AbstractState::split(int var, vector<int> wanted) {
    assert(node);
    // We can only refine for vars that can have at least two values.
    // The desired value has to be in the set of possible values.
    assert(wanted.size() >= 1);
    assert(values.count(var) > wanted.size());
    for (size_t i = 0; i < wanted.size(); ++i)
        assert(values.test(var, wanted[i]));

    Values v1_values(values);
    Values v2_values(values);

    v2_values.remove_all(var);
    for (size_t i = 0; i < wanted.size(); ++i) {
        // In v1 var can have all of the previous values except the wanted ones.
        v1_values.remove(var, wanted[i]);

        // In v2 var can only have the wanted values.
        v2_values.add(var, wanted[i]);
    }
    assert(v1_values.count(var) == values.count(var) - wanted.size());
    assert(v2_values.count(var) == wanted.size());

    // Update split tree.
    pair<Node *, Node *> new_nodes = node->split(var, wanted);

    AbstractState *v1 = new AbstractState(v1_values, new_nodes.first);
    AbstractState *v2 = new AbstractState(v2_values, new_nodes.second);

    // Check that the sets of possible values are now smaller.
    assert(this->is_abstraction_of(*v1));
    assert(this->is_abstraction_of(*v2));

    // Update transition system.
    update_incoming_arcs(var, v1, v2);
    update_outgoing_arcs(var, v1, v2);
    update_loops(var, v1, v2);

    // Since h-values only increase we can assign the h-value to the children.
    int h = node->get_h();
    v1->set_h(h);
    v2->set_h(h);

    return make_pair(v1, v2);
}

void AbstractState::add_arc(OperatorProxy op, AbstractState *other) {
    // Experiments showed that keeping the arcs sorted for faster removal
    // increases the overall processing time. Out of 30 domains it made no
    // difference for 10 domains, 17 domains preferred unsorted arcs and in
    // 3 domains performance was better with sorted arcs.
    // Inlining this method has no effect.
    assert(other != this);
    arcs_out.push_back(Arc(op, other));
    other->arcs_in.push_back(Arc(op, this));
}

void AbstractState::add_loop(OperatorProxy op) {
    loops.push_back(op);
}

void AbstractState::remove_arc(Arcs &arcs, OperatorProxy op, AbstractState *other) {
    auto pos = find(arcs.begin(), arcs.end(), Arc(op, other));
    assert(pos != arcs.end());
    swap(*pos, arcs.back());
    arcs.pop_back();
}

void AbstractState::remove_next_arc(OperatorProxy op, AbstractState *other) {
    remove_arc(arcs_out, op, other);
}

void AbstractState::remove_prev_arc(OperatorProxy op, AbstractState *other) {
    remove_arc(arcs_in, op, other);
}

bool AbstractState::is_abstraction_of(const State &conc_state) const {
    for (FactProxy fact : conc_state) {
        if (!values.test(fact.get_variable().get_id(), fact.get_value()))
            return false;
    }
    return true;
}

bool AbstractState::is_abstraction_of(const AbstractState &other) const {
    return values.abstracts(other.values);
}

void AbstractState::set_h(int dist) {
    assert(node);
    node->set_h(dist);
}

int AbstractState::get_h() const {
    assert(node);
    return node->get_h();
}
}
