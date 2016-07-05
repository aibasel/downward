#include "abstract_state.h"

#include "refinement_hierarchy.h"
#include "utils.h"

#include <algorithm>
#include <cassert>
#include <unordered_set>

using namespace std;

namespace cegar {
const int AbstractSearchInfo::UNDEFINED_OPERATOR = -1;

AbstractState::AbstractState(
    const TaskProxy &task_proxy, const Domains &domains, Node *node)
    : task_proxy(task_proxy),
      domains(domains),
      node(node) {
}

AbstractState::AbstractState(AbstractState &&other)
    : task_proxy(move(other.task_proxy)),
      domains(move(other.domains)),
      node(move(other.node)),
      incoming_arcs(move(other.incoming_arcs)),
      outgoing_arcs(move(other.outgoing_arcs)),
      loops(move(other.loops)),
      search_info(move(other.search_info)) {
}

int AbstractState::count(int var) const {
    return domains.count(var);
}

bool AbstractState::contains(FactProxy fact) const {
    return domains.test(fact.get_variable().get_id(), fact.get_value());
}

void AbstractState::add_arc(int op_id, AbstractState *other) {
    /*
      Experiments showed that keeping the arcs sorted for faster removal
      increases the overall processing time. Out of 30 domains it made no
      difference for 10 domains, 17 domains preferred unsorted arcs and in
      3 domains performance was better with sorted arcs.
      Inlining this method has no effect.
    */
    assert(other != this);
    outgoing_arcs.push_back(Arc(op_id, other));
    other->incoming_arcs.push_back(Arc(op_id, this));
}

void AbstractState::add_loop(int op_id) {
    loops.push_back(op_id);
}

void AbstractState::remove_arc(Arcs &arcs, int op_id, AbstractState *other) {
    auto pos = find(arcs.begin(), arcs.end(), Arc(op_id, other));
    assert(pos != arcs.end());
    swap(*pos, arcs.back());
    arcs.pop_back();
}

void AbstractState::remove_incoming_arc(int op_id, AbstractState *other) {
    remove_arc(incoming_arcs, op_id, other);
}

void AbstractState::remove_outgoing_arc(int op_id, AbstractState *other) {
    remove_arc(outgoing_arcs, op_id, other);
}

void AbstractState::split_incoming_arcs(int var, AbstractState *v1, AbstractState *v2) {
    /* Assume that the abstract state v has been split into v1 and v2.
       Now for all transitions u->v we need to add transitions u->v1,
       u->v2, or both. */
    for (const Arc &arc : incoming_arcs) {
        int op_id = arc.op_id;
        OperatorProxy op = task_proxy.get_operators()[op_id];
        AbstractState *u = arc.target;
        assert(u != this);
        int post = get_post(op, var);
        if (post == UNDEFINED_VALUE) {
            // op has no precondition and no effect on var.
            bool u_and_v1_intersect = u->domains_intersect(v1, var);
            if (u_and_v1_intersect) {
                u->add_arc(op_id, v1);
            }
            /* If the domains of u and v1 don't intersect, we must add
               the other arc and can avoid an intersection test. */
            if (!u_and_v1_intersect || u->domains_intersect(v2, var)) {
                u->add_arc(op_id, v2);
            }
        } else if (v1->domains.test(var, post)) {
            // op can only end in v1.
            u->add_arc(op_id, v1);
        } else {
            // op can only end in v2.
            assert(v2->domains.test(var, post));
            u->add_arc(op_id, v2);
        }
        u->remove_outgoing_arc(op_id, this);
    }
}

void AbstractState::split_outgoing_arcs(int var, AbstractState *v1, AbstractState *v2) {
    /* Assume that the abstract state v has been split into v1 and v2.
       Now for all transitions v->w we need to add transitions v1->w,
       v2->w, or both. */
    for (const Arc &arc : outgoing_arcs) {
        int op_id = arc.op_id;
        OperatorProxy op = task_proxy.get_operators()[op_id];
        AbstractState *w = arc.target;
        assert(w != this);
        int pre = get_pre(op, var);
        int post = get_post(op, var);
        if (post == UNDEFINED_VALUE) {
            assert(pre == UNDEFINED_VALUE);
            // op has no precondition and no effect on var.
            bool v1_and_w_intersect = v1->domains_intersect(w, var);
            if (v1_and_w_intersect) {
                v1->add_arc(op_id, w);
            }
            /* If the domains of v1 and w don't intersect, we must add
               the other arc and can avoid an intersection test. */
            if (!v1_and_w_intersect || v2->domains_intersect(w, var)) {
                v2->add_arc(op_id, w);
            }
        } else if (pre == UNDEFINED_VALUE) {
            // op has no precondition, but an effect on var.
            v1->add_arc(op_id, w);
            v2->add_arc(op_id, w);
        } else if (v1->domains.test(var, pre)) {
            // op can only start in v1.
            v1->add_arc(op_id, w);
        } else {
            // op can only start in v2.
            assert(v2->domains.test(var, pre));
            v2->add_arc(op_id, w);
        }
        w->remove_incoming_arc(op_id, this);
    }
}

void AbstractState::split_loops(int var, AbstractState *v1, AbstractState *v2) {
    /* Assume that the abstract state v has been split into v1 and v2.
       Now for all self-loops v->v we need to add one or two of the
       transitions v1->v1, v1->v2, v2->v1 and v2->v2. */
    for (int op_id : loops) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        int pre = get_pre(op, var);
        int post = get_post(op, var);
        if (pre == UNDEFINED_VALUE) {
            // op has no precondition on var --> it must start in v1 and v2.
            if (post == UNDEFINED_VALUE) {
                // op has no effect on var --> it must end in v1 and v2.
                v1->add_loop(op_id);
                v2->add_loop(op_id);
            } else if (v2->domains.test(var, post)) {
                // op must end in v2.
                v1->add_arc(op_id, v2);
                v2->add_loop(op_id);
            } else {
                // op must end in v1.
                assert(v1->domains.test(var, post));
                v1->add_loop(op_id);
                v2->add_arc(op_id, v1);
            }
        } else if (v1->domains.test(var, pre)) {
            // op must start in v1.
            assert(post != UNDEFINED_VALUE);
            if (v1->domains.test(var, post)) {
                // op must end in v1.
                v1->add_loop(op_id);
            } else {
                // op must end in v2.
                assert(v2->domains.test(var, post));
                v1->add_arc(op_id, v2);
            }
        } else {
            // op must start in v2.
            assert(v2->domains.test(var, pre));
            assert(post != UNDEFINED_VALUE);
            if (v1->domains.test(var, post)) {
                // op must end in v1.
                v2->add_arc(op_id, v1);
            } else {
                // op must end in v2.
                assert(v2->domains.test(var, post));
                v2->add_loop(op_id);
            }
        }
    }
}

pair<AbstractState *, AbstractState *> AbstractState::split(
    int var, const vector<int> &wanted) {
    int num_wanted = wanted.size();
    utils::unused_variable(num_wanted);
    // We can only split states in the refinement hierarchy (not artificial states).
    assert(node);
    // We can only refine for variables with at least two values.
    assert(num_wanted >= 1);
    assert(domains.count(var) > num_wanted);

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
    assert(v1_domains.count(var) == domains.count(var) - num_wanted);
    assert(v2_domains.count(var) == num_wanted);

    // Update refinement hierarchy.
    pair<Node *, Node *> new_nodes = node->split(var, wanted);

    AbstractState *v1 = new AbstractState(task_proxy, v1_domains, new_nodes.first);
    AbstractState *v2 = new AbstractState(task_proxy, v2_domains, new_nodes.second);

    assert(this->is_more_general_than(*v1));
    assert(this->is_more_general_than(*v2));

    // Update transition system.
    split_incoming_arcs(var, v1, v2);
    split_outgoing_arcs(var, v1, v2);
    split_loops(var, v1, v2);

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
        regressed_domains.set_single_value(var_id, precondition.get_value());
    }
    return AbstractState(task_proxy, regressed_domains, nullptr);
}

bool AbstractState::domains_intersect(const AbstractState *other, int var) const {
    return domains.intersects(other->domains, var);
}

bool AbstractState::includes(const State &concrete_state) const {
    for (FactProxy fact : concrete_state) {
        if (!domains.test(fact.get_variable().get_id(), fact.get_value()))
            return false;
    }
    return true;
}

bool AbstractState::is_more_general_than(const AbstractState &other) const {
    return domains.is_superset_of(other.domains);
}

void AbstractState::set_h_value(int new_h) {
    assert(node);
    node->increase_h_value_to(new_h);
}

int AbstractState::get_h_value() const {
    assert(node);
    return node->get_h_value();
}

AbstractState *AbstractState::get_trivial_abstract_state(
    const TaskProxy &task_proxy, Node *root_node) {
    AbstractState *abstract_state = new AbstractState(
        task_proxy, Domains(get_domain_sizes(task_proxy)), root_node);
    for (OperatorProxy op : task_proxy.get_operators()) {
        abstract_state->add_loop(op.get_id());
    }
    return abstract_state;
}

AbstractState AbstractState::get_abstract_state(
    const TaskProxy &task_proxy, const ConditionsProxy &conditions) {
    Domains domains(get_domain_sizes(task_proxy));
    for (FactProxy condition : conditions) {
        domains.set_single_value(condition.get_variable().get_id(), condition.get_value());
    }
    return AbstractState(task_proxy, domains, nullptr);
}
}
