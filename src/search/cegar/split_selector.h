#ifndef CEGAR_SPLIT_SELECTOR_H
#define CEGAR_SPLIT_SELECTOR_H

#include "utils.h"

#include "../task_proxy.h"

#include <memory>
#include <vector>

class AdditiveHeuristic;

namespace cegar {
class AbstractState;

// Strategies for selecting a flaw in case there are multiple possibilities.
enum class PickSplit {
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

struct Split {
    const int var_id;
    const std::vector<int> values;

    Split(int var_id, std::vector<int> &&values)
        : var_id(var_id), values(values) {
    }
};

using Splits = std::vector<Split>;

class SplitSelector {
    const std::shared_ptr<AbstractTask> task;
    const TaskProxy task_proxy;
    std::shared_ptr<AdditiveHeuristic> additive_heuristic;

    const PickSplit pick;

    int get_constrainedness(const AbstractState &state, int var_id) const;
    double get_refinedness(const AbstractState &state, int var_id) const;
    int get_hadd_value(int var_id, int value) const;
    int get_extreme_hadd_value(int var_id, const std::vector<int> &values) const;

    double rate_split(const AbstractState &state, const Split &split) const;

public:
    SplitSelector(std::shared_ptr<AbstractTask> task, PickSplit pick);
    ~SplitSelector() = default;

    const Split &pick_split(const AbstractState &state, const Splits &splits) const;
};
}

#endif
