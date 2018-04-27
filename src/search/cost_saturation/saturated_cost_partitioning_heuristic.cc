#include "saturated_cost_partitioning_heuristic.h"

#include "abstraction.h"
#include "cost_partitioned_heuristic.h"
#include "cost_partitioning_collection_generator.h"
#include "max_cost_partitioning_heuristic.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"

using namespace std;

namespace cost_saturation {
static CostPartitionedHeuristic compute_saturated_cost_partitioning(
    const Abstractions &abstractions,
    const vector<int> &order,
    const vector<int> &costs) {
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
        cp_heuristic.add_lookup_table_if_nonzero(pos, move(h_values));
        reduce_costs(remaining_costs, saturated_costs);
        if (debug) {
            cout << "Remaining costs: ";
            print_indexed_vector(remaining_costs);
        }
    }
    return cp_heuristic;
}

static Heuristic *get_max_cp_heuristic(
    options::OptionParser &parser, CPFunction cp_function) {
    prepare_parser_for_cost_partitioning_heuristic(parser);
    add_cost_partitioning_collection_options_to_parser(parser);

    options::Options opts = parser.parse();
    if (parser.help_mode())
        return nullptr;

    if (parser.dry_run())
        return nullptr;

    shared_ptr<AbstractTask> task = opts.get<shared_ptr<AbstractTask>>("transform");
    TaskProxy task_proxy(*task);
    vector<int> costs = task_properties::get_operator_costs(task_proxy);
    Abstractions abstractions = generate_abstractions(
        task, opts.get_list<shared_ptr<AbstractionGenerator>>("abstraction_generators"));
    return new MaxCostPartitioningHeuristic(
        opts,
        move(abstractions),
        get_cp_collection_generator_from_options(opts).get_cost_partitionings(
            task_proxy, abstractions, costs, cp_function));
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Saturated cost partitioning heuristic",
        "");
    return get_max_cp_heuristic(parser, compute_saturated_cost_partitioning);
}

static Plugin<Heuristic> _plugin("saturated_cost_partitioning", _parse);
}
