#include "fts_factory.h"

#include "factored_transition_system.h"
#include "transition_system.h"

#include "../utilities.h"

#include <vector>

using namespace std;


unique_ptr<FactoredTransitionSystem> create_factored_transition_system(
    const TaskProxy &task_proxy, shared_ptr<Labels> labels) {
    vector<TransitionSystem *> transition_systems;
    TransitionSystem::build_atomic_transition_systems(
        task_proxy, transition_systems, labels);
    return make_unique_ptr<FactoredTransitionSystem>(
        move(transition_systems));
}
