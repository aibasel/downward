#include "max_cost_partitioning_heuristic.h"

#include "abstraction.h"
#include "cost_partitioning_heuristic.h"
#include "utils.h"

#include "../utils/logging.h"

using namespace std;

namespace cost_saturation {
static int compute_max_h(
    const vector<CostPartitioningHeuristic> &cp_heuristics,
    const vector<int> &abstract_state_ids) {
    int max_h = 0;
    for (const CostPartitioningHeuristic &cp_heuristic : cp_heuristics) {
        int sum_h = cp_heuristic.compute_heuristic(abstract_state_ids);
        max_h = max(max_h, sum_h);
    }
    return max_h;
}

static void log_info_about_stored_lookup_tables(
    const Abstractions &abstractions,
    const vector<CostPartitioningHeuristic> &cp_heuristics) {
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
}

static void delete_useless_abstractions(
    const vector<CostPartitioningHeuristic> &cp_heuristics,
    const UnsolvabilityHeuristic &unsolvability_heuristic,
    Abstractions &abstractions) {
    int num_abstractions = abstractions.size();

    // Collect IDs of useful abstractions.
    vector<bool> useful_abstractions(num_abstractions, false);
    unsolvability_heuristic.mark_useful_abstractions(useful_abstractions);
    for (const auto &cp_heuristic : cp_heuristics) {
        cp_heuristic.mark_useful_abstractions(useful_abstractions);
    }

    // Delete useless abstractions.
    for (int i = 0; i < num_abstractions; ++i) {
        if (!useful_abstractions[i]) {
            abstractions[i] = nullptr;
        }
    }
}

MaxCostPartitioningHeuristic::MaxCostPartitioningHeuristic(
    const options::Options &opts,
    Abstractions &&abstractions_,
    vector<CostPartitioningHeuristic> &&cp_heuristics_,
    UnsolvabilityHeuristic &&unsolvability_heuristic_)
    : Heuristic(opts),
      abstractions(move(abstractions_)),
      cp_heuristics(move(cp_heuristics_)),
      unsolvability_heuristic(move(unsolvability_heuristic_)) {
    int num_abstractions = abstractions.size();
    log_info_about_stored_lookup_tables(abstractions, cp_heuristics);

    delete_useless_abstractions(
        cp_heuristics, unsolvability_heuristic, abstractions);

    int num_useful_abstractions = num_abstractions - count(
        abstractions.begin(), abstractions.end(), nullptr);
    utils::Log() << "Useful abstractions: " << num_useful_abstractions << "/"
                 << num_abstractions << " = "
                 << static_cast<double>(num_useful_abstractions) / num_abstractions
                 << endl;

    // Delete transition systems. They are not needed during the search.
    for (auto &abstraction : abstractions) {
        if (abstraction) {
            abstraction->remove_transition_system();
        }
    }
}

int MaxCostPartitioningHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    return compute_heuristic(state);
}

int MaxCostPartitioningHeuristic::compute_heuristic(const State &state) const {
    vector<int> abstract_state_ids = get_abstract_state_ids(abstractions, state);
    if (unsolvability_heuristic.is_unsolvable(abstract_state_ids)) {
        return DEAD_END;
    }
    return compute_max_h(cp_heuristics, abstract_state_ids);
}
}
