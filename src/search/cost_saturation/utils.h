#ifndef COST_SATURATION_UTILS_H
#define COST_SATURATION_UTILS_H

#include "abstraction.h"
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

template<typename AbstractionsOrFunctions>
std::vector<int> get_abstract_state_ids(
    const AbstractionsOrFunctions &abstractions, const State &state) {
    std::vector<int> abstract_state_ids;
    abstract_state_ids.reserve(abstractions.size());
    for (auto &abstraction : abstractions) {
        if (abstraction) {
            // Only add local state IDs for useful abstractions.
            abstract_state_ids.push_back(abstraction->get_abstract_state_id(state));
        } else {
            // Add dummy value if abstraction will never be used.
            abstract_state_ids.push_back(-1);
        }
    }
    return abstract_state_ids;
}

extern void reduce_costs(
    std::vector<int> &remaining_costs, const std::vector<int> &saturated_costs);
}

#endif
