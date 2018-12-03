#include "max_cost_partitioning_heuristic.h"

#include "abstraction.h"
#include "cost_partitioning_heuristic.h"
#include "utils.h"

#include "../option_parser.h"

#include "../utils/logging.h"

using namespace std;

namespace cost_saturation {
static int compute_max_h(
    const vector<CostPartitioningHeuristic> &cp_heuristics,
    const vector<int> &abstract_state_ids) {
    int max_h = 0;
    for (const CostPartitioningHeuristic &cp_heuristic : cp_heuristics) {
        int sum_h = cp_heuristic.compute_heuristic(abstract_state_ids);
        if (sum_h == INF) {
            return INF;
        }
        max_h = max(max_h, sum_h);
    }
    return max_h;
}

MaxCostPartitioningHeuristic::MaxCostPartitioningHeuristic(
    const Options &opts,
    Abstractions &&abstractions_,
    vector<CostPartitioningHeuristic> &&cp_heuristics_)
    : Heuristic(opts),
      abstractions(move(abstractions_)),
      cp_heuristics(move(cp_heuristics_)) {
    int num_abstractions = abstractions.size();

    // Print statistics about the number of lookup tables.
    int num_lookup_tables = num_abstractions * cp_heuristics.size();
    int num_stored_lookup_tables = 0;
    for (const auto &cp_heuristic: cp_heuristics) {
        num_stored_lookup_tables += cp_heuristic.get_num_lookup_tables();
    }
    utils::Log() << "Stored lookup tables: " << num_stored_lookup_tables << "/"
                 << num_lookup_tables << " = "
                 << num_stored_lookup_tables / static_cast<double>(num_lookup_tables)
                 << endl;

    // Print statistics about the number of stored values.
    int num_stored_values = 0;
    for (const auto &cp_heuristic : cp_heuristics) {
        num_stored_values += cp_heuristic.get_num_heuristic_values();
    }
    int num_total_values = 0;
    for (const auto &abstraction : abstractions) {
        num_total_values += abstraction->get_num_states();
    }
    num_total_values *= cp_heuristics.size();
    utils::Log() << "Stored values: " << num_stored_values << "/"
                 << num_total_values << " = "
                 << num_stored_values / static_cast<double>(num_total_values) << endl;

    // Collect IDs of useful abstractions.
    vector<bool> useful_abstractions(num_abstractions, false);
    for (const auto &cp_heuristic : cp_heuristics) {
        cp_heuristic.mark_useful_heuristics(useful_abstractions);
    }
    int num_useful_abstractions = count(
        useful_abstractions.begin(), useful_abstractions.end(), true);
    utils::Log() << "Useful abstractions: " << num_useful_abstractions << "/"
                 << num_abstractions << " = "
                 << static_cast<double>(num_useful_abstractions) / num_abstractions
                 << endl;

    // Delete useless abstractions.
    for (int i = 0; i < num_abstractions; ++i) {
        if (!useful_abstractions[i]) {
            abstractions[i] = nullptr;
        }
    }

    // Delete transition systems since they are not required during the search.
    for (auto &abstraction : abstractions) {
        if (abstraction) {
            abstraction->remove_transition_system();
        }
    }
}

MaxCostPartitioningHeuristic::~MaxCostPartitioningHeuristic() {
}

int MaxCostPartitioningHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    return compute_heuristic(state);
}

int MaxCostPartitioningHeuristic::compute_heuristic(const State &state) const {
    vector<int> abstract_state_ids = get_abstract_state_ids(abstractions, state);
    int max_h = compute_max_h(cp_heuristics, abstract_state_ids);
    if (max_h == INF) {
        return DEAD_END;
    }
    return max_h;
}
}
