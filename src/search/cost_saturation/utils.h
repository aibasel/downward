#ifndef COST_SATURATION_UTILS_H
#define COST_SATURATION_UTILS_H

#include "types.h"

#include <vector>

class AbstractTask;
class State;

namespace options {
class OptionParser;
}

namespace cost_saturation {
class AbstractionGenerator;

extern Abstractions generate_abstractions(
    const std::shared_ptr<AbstractTask> &task,
    const std::vector<std::shared_ptr<AbstractionGenerator>> &abstraction_generators);

extern std::vector<int> get_default_order(int num_abstractions);

extern std::vector<int> get_local_state_ids(
    const Abstractions &abstractions, const State &state);

extern void reduce_costs(
    std::vector<int> &remaining_costs,
    const std::vector<int> &saturated_costs);
}

#endif
