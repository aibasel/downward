#include "fts_factory.h"

#include "factored_transition_system.h"
#include "transition_system.h"

#include "../task_proxy.h"
#include "../utilities.h"

#include <cassert>
#include <vector>

using namespace std;


FactoredTransitionSystem create_factored_transition_system(
    const TaskProxy &task_proxy, shared_ptr<Labels> labels) {
    vector<TransitionSystem *> transition_systems;
    int num_vars = task_proxy.get_variables().size();
    assert(num_vars >= 1);
    transition_systems.reserve(num_vars * 2 - 1);
    TransitionSystem::build_atomic_transition_systems(
        task_proxy, transition_systems, labels);
    return FactoredTransitionSystem(move(transition_systems));
}
