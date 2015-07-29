#include "factored_transition_system.h"

using namespace std;


FactoredTransitionSystem::FactoredTransitionSystem(
    vector<TransitionSystem *> && transition_systems)
    : transition_systems(transition_systems) {
}

FactoredTransitionSystem::FactoredTransitionSystem(FactoredTransitionSystem && other)
    : transition_systems(move(other.transition_systems)) {
    /*
      This is just a default move constructor. Unfortunately Visual
      Studio does not support "= default" for move construction or
      move assignment as of this writing.
    */
}

FactoredTransitionSystem::~FactoredTransitionSystem() {
}
