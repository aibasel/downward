#ifndef COST_SATURATION_UTILS_H
#define COST_SATURATION_UTILS_H

#include "types.h"

#include <vector>

class AbstractTask;
class State;

namespace cost_saturation {
class AbstractionGenerator;

extern Abstractions generate_abstractions(
    const std::shared_ptr<AbstractTask> &task,
    const std::vector<std::shared_ptr<AbstractionGenerator>> &abstraction_generators);

extern Order get_default_order(int num_abstractions);

extern int compute_max_h_with_statistics(
    const CPHeuristics &cp_heuristics,
    const std::vector<int> &abstract_state_ids,
    std::vector<int> &num_best_order);

extern std::vector<int> get_abstract_state_ids(
    const Abstractions &abstractions, const State &state);

extern std::vector<int> get_abstract_state_ids(
    const AbstractionFunctions &abstraction_functions, const State &state);

extern void reduce_costs(
    std::vector<int> &remaining_costs, const std::vector<int> &saturated_costs);
}

#endif
