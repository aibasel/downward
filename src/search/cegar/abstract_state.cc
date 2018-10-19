#include "abstract_state.h"

#include "refinement_hierarchy.h"
#include "utils.h"

#include <algorithm>
#include <cassert>
#include <unordered_set>

using namespace std;

namespace cegar {
const int AbstractSearchInfo::UNDEFINED_OPERATOR = -1;

AbstractState::AbstractState(const Domains &domains, Node *node)
    : domains(domains),
      node(node) {
}

AbstractState::AbstractState(AbstractState &&other)
    : domains(move(other.domains)),
      node(move(other.node)),
      incoming_transitions(move(other.incoming_transitions)),
      outgoing_transitions(move(other.outgoing_transitions)),
      loops(move(other.loops)),
      search_info(move(other.search_info)) {
}

int AbstractState::count(int var) const {
    return domains.count(var);
}

bool AbstractState::contains(int var, int value) const {
    return domains.test(var, value);
}

void AbstractState::add_outgoing_transition(int op_id, AbstractState *target) {
    assert(target != this);
    outgoing_transitions.emplace_back(op_id, target);
}

void AbstractState::add_incoming_transition(int op_id, AbstractState *src) {
    assert(src != this);
    incoming_transitions.emplace_back(op_id, src);
}

void AbstractState::add_loop(int op_id) {
    loops.push_back(op_id);
}

void AbstractState::remove_non_looping_transition(
    Transitions &transitions, int op_id, AbstractState *other) {
    auto pos = find(
        transitions.begin(), transitions.end(), Transition(op_id, other));
    assert(pos != transitions.end());
    swap(*pos, transitions.back());
    transitions.pop_back();
}

void AbstractState::remove_incoming_transition(int op_id, AbstractState *other) {
    remove_non_looping_transition(incoming_transitions, op_id, other);
}

void AbstractState::remove_outgoing_transition(int op_id, AbstractState *other) {
    remove_non_looping_transition(outgoing_transitions, op_id, other);
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

    AbstractState *v1 = new AbstractState(v1_domains, new_nodes.first);
    AbstractState *v2 = new AbstractState(v2_domains, new_nodes.second);

    assert(this->is_more_general_than(*v1));
    assert(this->is_more_general_than(*v2));

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
    return AbstractState(regressed_domains, nullptr);
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
        Domains(get_domain_sizes(task_proxy)), root_node);
    return abstract_state;
}

AbstractState AbstractState::get_abstract_state(
    const TaskProxy &task_proxy, const ConditionsProxy &conditions) {
    Domains domains(get_domain_sizes(task_proxy));
    for (FactProxy condition : conditions) {
        domains.set_single_value(condition.get_variable().get_id(), condition.get_value());
    }
    return AbstractState(domains, nullptr);
}
}
