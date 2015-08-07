#ifndef MERGE_AND_SHRINK_FACTORED_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_FACTORED_TRANSITION_SYSTEM_H

#include <vector>

class TransitionSystem;


class FactoredTransitionSystem {
    // Set of all transition systems. Entries with nullptr have been merged.
    std::vector<TransitionSystem *> transition_systems;
public:
    explicit FactoredTransitionSystem(
        std::vector<TransitionSystem *> && transition_systems);
    FactoredTransitionSystem(FactoredTransitionSystem && other);
    ~FactoredTransitionSystem();

    // No copying or assignment.
    FactoredTransitionSystem(const FactoredTransitionSystem &) = delete;
    FactoredTransitionSystem &operator=(
        const FactoredTransitionSystem &) = delete;

    TransitionSystem * &operator[](int index) {
        return transition_systems[index];
    }

    const TransitionSystem *operator[](int index) const {
        return transition_systems[index];
    }

    // temporary method while we're transitioning from the old code;
    // TODO: remove
    std::vector<TransitionSystem *> &get_vector() {
        return transition_systems;
    }
};

#endif
