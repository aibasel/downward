#ifndef CEGAR_SPLIT_SELECTOR_H
#define CEGAR_SPLIT_SELECTOR_H

#include "utils.h"

#include "../task_proxy.h"

#include <memory>
#include <vector>

namespace AdditiveHeuristic {
class AdditiveHeuristic;
}

namespace CEGAR {
class AbstractState;

// Strategies for selecting a split in case there are multiple possibilities.
enum class PickSplit {
    RANDOM,
    // Number of values that land in the state whose h-value is probably raised.
    MIN_UNWANTED,
    MAX_UNWANTED,
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

class SplitSelector {
    const std::shared_ptr<AbstractTask> task;
    const TaskProxy task_proxy;
    std::unique_ptr<AdditiveHeuristic::AdditiveHeuristic> additive_heuristic;

    const PickSplit pick;

    int get_num_unwanted_values(const AbstractState &state, const Split &split) const;
    double get_refinedness(const AbstractState &state, int var_id) const;
    int get_hadd_value(int var_id, int value) const;
    int get_extreme_hadd_value(int var_id, const std::vector<int> &values) const;

    double rate_split(const AbstractState &state, const Split &split) const;

public:
    SplitSelector(std::shared_ptr<AbstractTask> task, PickSplit pick);
    ~SplitSelector();

    const Split &pick_split(const AbstractState &state,
                            const std::vector<Split> &splits) const;
};
}

#endif
