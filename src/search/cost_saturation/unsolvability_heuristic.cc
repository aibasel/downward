#include "max_cost_partitioning_heuristic.h"

#include "abstraction.h"
#include "utils.h"

#include "../option_parser.h"

#include "../task_utils/task_properties.h"

using namespace std;

namespace cost_saturation {
static vector<bool> get_unsolvable_states(const Abstraction &abstraction, int num_ops) {
    // Note: we could use a simple queue instead of a priority queue for this.
    vector<bool> unsolvable_states(abstraction.get_num_states(), false);
    vector<int> unit_costs(num_ops, 1);
    vector<int> goal_distances = abstraction.compute_goal_distances(unit_costs);
    for (size_t i = 0; i < goal_distances.size(); ++i) {
        if (goal_distances[i] == INF) {
            unsolvable_states[i] = true;
        }
    }
    return unsolvable_states;
}

UnsolvabilityHeuristic::UnsolvabilityHeuristic(
    const Abstractions &abstractions, int num_operators) {
    for (size_t i = 0; i < abstractions.size(); ++i) {
        vector<bool> unsolvable = get_unsolvable_states(*abstractions[i], num_operators);
        if (any_of(unsolvable.begin(), unsolvable.end(), [](bool b) {return b;})) {
            unsolvable_states.emplace_back(i, move(unsolvable));
        }
    }
}

bool UnsolvabilityHeuristic::is_unsolvable(const vector<int> &abstract_state_ids) const {
    for (const auto &info : unsolvable_states) {
        if (info.unsolvable_states[abstract_state_ids[info.abstraction_id]]) {
            return true;
        }
    }
    return false;
}

void UnsolvabilityHeuristic::mark_useful_abstractions(
    vector<bool> &useful_abstractions) const {
    for (const auto &info : unsolvable_states) {
        useful_abstractions[info.abstraction_id] = true;
    }
}
}
