#include "abstract_state.h"

#include "split_tree.h"
#include "utils.h"

#include "../task_proxy.h"

#include <algorithm>
#include <cassert>
#include <unordered_set>

using namespace std;

namespace CEGAR {
AbstractState::AbstractState(const Domains &domains, Node *node)
    : domains(domains),
      node(node) {
}

size_t AbstractState::count(int var) const {
    return domains.count(var);
}

bool AbstractState::contains(FactProxy fact) const {
    return domains.test(fact.get_variable().get_id(), fact.get_value());
}

void AbstractState::add_arc(OperatorProxy op, AbstractState *other) {
    /*
      Experiments showed that keeping the arcs sorted for faster removal
      increases the overall processing time. Out of 30 domains it made no
      difference for 10 domains, 17 domains preferred unsorted arcs and in
      3 domains performance was better with sorted arcs.
      Inlining this method has no effect.
    */
    assert(other != this);
    outgoing_arcs.push_back(Arc(op, other));
    other->incoming_arcs.push_back(Arc(op, this));
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

void AbstractState::remove_incoming_arc(OperatorProxy op, AbstractState *other) {
    remove_arc(incoming_arcs, op, other);
}

void AbstractState::remove_outgoing_arc(OperatorProxy op, AbstractState *other) {
    remove_arc(outgoing_arcs, op, other);
}

void AbstractState::update_incoming_arcs(int var, AbstractState *v1, AbstractState *v2) {
    for (auto arc : incoming_arcs) {
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
        } else if (v2->domains.test(var, post)) {
            u->add_arc(op, v2);
        } else {
            u->add_arc(op, v1);
        }
        u->remove_outgoing_arc(op, this);
    }
}

void AbstractState::update_outgoing_arcs(int var, AbstractState *v1, AbstractState *v2) {
    for (auto arc : outgoing_arcs) {
        OperatorProxy op = arc.first;
        AbstractState *w = arc.second;
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
        } else if (v2->domains.test(var, pre)) {
            v2->add_arc(op, w);
        } else {
            v1->add_arc(op, w);
        }
        w->remove_incoming_arc(op, this);
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
            } else if (v2->domains.test(var, post)) {
                v1->add_arc(op, v2);
                v2->add_loop(op);
            } else {
                assert(v1->domains.test(var, post));
                v1->add_loop(op);
                v2->add_arc(op, v1);
            }
        } else if (v2->domains.test(var, pre)) {
            assert(post != UNDEFINED);
            if (v2->domains.test(var, post)) {
                v2->add_loop(op);
            } else {
                assert(v1->domains.test(var, post));
                v2->add_arc(op, v1);
            }
        } else if (v2->domains.test(var, post)) {
            v1->add_arc(op, v2);
        } else {
            assert(v1->domains.test(var, post));
            v1->add_loop(op);
        }
    }
}

pair<AbstractState *, AbstractState *> AbstractState::split(
    int var, const vector<int> &wanted) {
    // We can only split states in the refinement hierarchy (not artificial states).
    assert(node);
    // We can only refine for variables with at least two values.
    assert(wanted.size() >= 1);
    assert(domains.count(var) > wanted.size());

    Domains v1_domains(domains);
    Domains v2_domains(domains);

    v2_domains.remove_all(var);
    for (int value : wanted) {
        // The wanted value has to be in the set of possible values.
        assert(domains.test(var, value));

        // In v1 var can have all of the previous values except the wanted ones.
        v1_domains.remove(var, value);

        // In v2 var can only have the wanted values.
        v2_domains.add(var, value);
    }
    assert(v1_domains.count(var) == domains.count(var) - wanted.size());
    assert(v2_domains.count(var) == wanted.size());

    // Update refinement hierarchy.
    pair<Node *, Node *> new_nodes = node->split(var, wanted);

    AbstractState *v1 = new AbstractState(v1_domains, new_nodes.first);
    AbstractState *v2 = new AbstractState(v2_domains, new_nodes.second);

    assert(this->is_abstraction_of(*v1));
    assert(this->is_abstraction_of(*v2));

    // Update transition system.
    update_incoming_arcs(var, v1, v2);
    update_outgoing_arcs(var, v1, v2);
    update_loops(var, v1, v2);

    // Since h-values only increase we can assign the h-value to the children.
    int h = node->get_h_value();
    v1->set_h_value(h);
    v2->set_h_value(h);

    return make_pair(v1, v2);
}

AbstractState AbstractState::regress(OperatorProxy op) const {
    Domains regressed_domains = domains;
    for (EffectProxy effect : op.get_effects()) {
        int var_id = effect.get_fact().get_variable().get_id();
        regressed_domains.add_all(var_id);
    }
    for (FactProxy precondition : op.get_preconditions()) {
        int var_id = precondition.get_variable().get_id();
        regressed_domains.set(var_id, precondition.get_value());
    }
    return AbstractState(regressed_domains, nullptr);
}

bool AbstractState::domains_intersect(const AbstractState *other, int var) const {
    return domains.intersects(other->domains, var);
}

bool AbstractState::is_abstraction_of(const State &conc_state) const {
    for (FactProxy fact : conc_state) {
        if (!domains.test(fact.get_variable().get_id(), fact.get_value()))
            return false;
    }
    return true;
}

bool AbstractState::is_abstraction_of(const AbstractState &other) const {
    return domains.is_superset_of(other.domains);
}

void AbstractState::set_h_value(int new_h) {
    assert(node);
    node->set_h_value(new_h);
}

int AbstractState::get_h_value() const {
    assert(node);
    return node->get_h_value();
}

AbstractState *AbstractState::get_trivial_abstract_state(
    TaskProxy task_proxy, Node *root_node) {
    return new AbstractState(Domains(get_domain_sizes(task_proxy)), root_node);
}

AbstractState AbstractState::get_abstract_state(
    TaskProxy task_proxy, const ConditionsProxy &conditions) {
    Domains domains(get_domain_sizes(task_proxy));
    for (FactProxy condition : conditions) {
        domains.set(condition.get_variable().get_id(), condition.get_value());
    }
    return AbstractState(domains, nullptr);
}
}
