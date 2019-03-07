#include "abstract_state.h"

#include "refinement_hierarchy.h"
#include "utils.h"

#include <algorithm>
#include <cassert>
#include <unordered_set>

using namespace std;

namespace cegar {
AbstractState::AbstractState(int state_id, NodeID node_id, Domains &&domains)
    : state_id(state_id),
      node_id(node_id),
      domains(move(domains)) {
}

AbstractState::AbstractState(AbstractState &&other)
    : state_id(other.state_id),
      node_id(other.node_id),
      domains(move(other.domains)) {
}

int AbstractState::count(int var) const {
    return domains.count(var);
}

bool AbstractState::contains(int var, int value) const {
    return domains.test(var, value);
}

pair<Domains, Domains> AbstractState::split_domain(
    int var, const vector<int> &wanted) {
    int num_wanted = wanted.size();
    utils::unused_variable(num_wanted);
    // We can only split states in the refinement hierarchy (not artificial states).
    assert(node_id != UNDEFINED);
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
    return make_pair(v1_domains, v2_domains);
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
    return AbstractState(UNDEFINED, UNDEFINED, move(regressed_domains));
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

bool AbstractState::includes(const AbstractState &other) const {
    return domains.is_superset_of(other.domains);
}

int AbstractState::get_id() const {
    return state_id;
}

NodeID AbstractState::get_node_id() const {
    return node_id;
}

AbstractState *AbstractState::get_trivial_abstract_state(
    const vector<int> &domain_sizes) {
    return new AbstractState(0, 0, Domains(domain_sizes));
}

AbstractState AbstractState::get_cartesian_set(
    const vector<int> &domain_sizes, const ConditionsProxy &conditions) {
    Domains domains(domain_sizes);
    for (FactProxy condition : conditions) {
        domains.set_single_value(condition.get_variable().get_id(), condition.get_value());
    }
    return AbstractState(UNDEFINED, UNDEFINED, move(domains));
}
}
