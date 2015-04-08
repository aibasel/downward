#ifndef CEGAR_FLAW_SELECTOR_H
#define CEGAR_FLAW_SELECTOR_H

#include "utils.h"

#include "../task_proxy.h"

#include <memory>
#include <vector>

class AdditiveHeuristic;

namespace cegar {
class AbstractState;

// Strategies for selecting a flaw in case there are multiple possibilities.
enum class PickFlaw {
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
    const TaskProxy task_proxy;
    std::shared_ptr<AdditiveHeuristic> additive_heuristic;

    PickFlaw pick;

    int get_constrainedness(const AbstractState &state, int var_id) const;
    double get_refinedness(const AbstractState &state, int var_id) const;
    int get_hadd_value(int var_id, int value) const;
    int get_extreme_hadd_value(int var_id, const std::vector<int> &values) const;

    double rate_flaw(const AbstractState &state, const Flaw &flaw) const;

public:
    FlawSelector(TaskProxy task_proxy, PickFlaw pick);
    ~FlawSelector() = default;

    FlawSelector(const FlawSelector &) = delete;
    FlawSelector &operator=(const FlawSelector &) = delete;

    const Flaw &pick_flaw(const AbstractState &state, const Flaws &conditions) const;
};
}

#endif
