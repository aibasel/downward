#include "saturated_cost_partitioning_heuristic.h"

#include "abstraction.h"
#include "cost_partitioned_heuristic.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace cost_saturation {
static CostPartitionedHeuristic compute_saturated_cost_partitioning(
    const Abstractions &abstractions,
    const vector<int> &order,
    const vector<int> &costs,
    bool sparse) {
    const bool debug = false;
    assert(abstractions.size() == order.size());
    CostPartitionedHeuristic cp_heuristic;
    vector<int> remaining_costs = costs;
    for (int pos : order) {
        const Abstraction &abstraction = *abstractions[pos];
        auto pair = abstraction.compute_goal_distances_and_saturated_costs(
            remaining_costs);
        vector<int> &h_values = pair.first;
        vector<int> &saturated_costs = pair.second;
        if (debug) {
            cout << "Heuristic values: ";
            print_indexed_vector(h_values);
            cout << "Saturated costs: ";
            print_indexed_vector(saturated_costs);
        }
        cp_heuristic.add_cp_heuristic_values(pos, move(h_values), sparse);
        reduce_costs(remaining_costs, saturated_costs);
        if (debug) {
            cout << "Remaining costs: ";
            print_indexed_vector(remaining_costs);
        }
    }
    return cp_heuristic;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Saturated cost partitioning heuristic",
        "");
    return get_max_cp_heuristic(parser, compute_saturated_cost_partitioning);
}

static Plugin<Heuristic> _plugin("saturated_cost_partitioning", _parse);
}
