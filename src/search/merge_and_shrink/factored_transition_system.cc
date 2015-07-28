#include "factored_transition_system.h"

using namespace std;


FactoredTransitionSystem::FactoredTransitionSystem(
    vector<TransitionSystem *> && transition_systems)
    : transition_systems(transition_systems) {
}

FactoredTransitionSystem::~FactoredTransitionSystem() {
}
