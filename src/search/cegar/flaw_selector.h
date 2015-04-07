#ifndef CEGAR_FLAW_SELECTOR_H
#define CEGAR_FLAW_SELECTOR_H

#include "utils.h"

#include "../task_proxy.h"

#include <memory>

class AdditiveHeuristic;

namespace cegar {
class AbstractState;

// In case there are multiple unment conditions, how do we choose the next one?
enum PickStrategy {
    RANDOM,
    // Number of remaining values for each variable.
    // "Constrainment" is bigger if there are less remaining possible values.
    MIN_CONSTRAINED,
    MAX_CONSTRAINED,
    // Refinement: - (remaining_values / original_domain_size)
    MIN_REFINED,
    MAX_REFINED,
    // Compare the h^add(s_0) values of the facts.
    MIN_HADD,
    MAX_HADD
};

class FlawSelector {
private:
    const TaskProxy task_proxy;
    std::shared_ptr<AdditiveHeuristic> additive_heuristic;

    // How to pick the next flaw in case of multiple possibilities.
    PickStrategy pick;

public:
    FlawSelector(TaskProxy task_proxy, PickStrategy pick);
    ~FlawSelector() = default;

    FlawSelector(const FlawSelector &) = delete;
    FlawSelector &operator=(const FlawSelector &) = delete;

    // Pick a possible flaw in case of multiple possibilities.
    int pick_flaw_index(const AbstractState &state, const Flaws &conditions) const;
};
}

#endif
