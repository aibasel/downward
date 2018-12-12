#include "abstract_state.h"

#include "refinement_hierarchy.h"
#include "utils.h"

#include <algorithm>
#include <cassert>
#include <unordered_set>

using namespace std;

namespace cegar {
AbstractState::AbstractState(const Domains &domains, Node *node)
    : domains(domains),
      node(node) {
}

AbstractState::AbstractState(AbstractState &&other)
    : domains(move(other.domains)),
      node(move(other.node)) {
}

int AbstractState::count(int var) const {
    return domains.count(var);
}

bool AbstractState::contains(int var, int value) const {
    return domains.test(var, value);
}

pair<AbstractState *, AbstractState *> AbstractState::split(
    int var, const vector<int> &wanted, int v1_id, int v2_id) {
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
    pair<Node *, Node *> new_nodes = node->split(
        var, wanted, v1_id, v2_id);

    AbstractState *v1 = new AbstractState(v1_domains, new_nodes.first);
    AbstractState *v2 = new AbstractState(v2_domains, new_nodes.second);

    assert(this->is_more_general_than(*v1));
    assert(this->is_more_general_than(*v2));

    return make_pair(v1, v2);
}

AbstractState AbstractState::regress(const OperatorProxy &op) const {
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

bool AbstractState::includes(const vector<FactPair> &facts) const {
    for (const FactPair &fact : facts) {
        if (!domains.test(fact.var, fact.value))
            return false;
    }
    return true;
}

bool AbstractState::is_more_general_than(const AbstractState &other) const {
    return domains.is_superset_of(other.domains);
}

int AbstractState::get_id() const {
    assert(node);
    return node->get_state_id();
}

AbstractState *AbstractState::get_trivial_abstract_state(
    const vector<int> &domain_sizes, Node *root_node) {
    return new AbstractState(Domains(domain_sizes), root_node);
}

AbstractState AbstractState::get_cartesian_set(
    const vector<int> &domain_sizes, const ConditionsProxy &conditions) {
    Domains domains(domain_sizes);
    for (FactProxy condition : conditions) {
        domains.set_single_value(condition.get_variable().get_id(), condition.get_value());
    }
    return AbstractState(domains, nullptr);
}
}
