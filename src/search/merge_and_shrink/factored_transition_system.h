#ifndef MERGE_AND_SHRINK_FACTORED_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_FACTORED_TRANSITION_SYSTEM_H

#include <vector>

class TransitionSystem;

class FactoredTransitionSystem {
    std::vector<TransitionSystem *> transition_systems;
public:
    explicit FactoredTransitionSystem(
        std::vector<TransitionSystem *> &&transition_systems);
    ~FactoredTransitionSystem();

    // No copying or assignment.
    FactoredTransitionSystem(const FactoredTransitionSystem &) = delete;
    FactoredTransitionSystem& operator=(
        const FactoredTransitionSystem &) = delete;
};

#endif
