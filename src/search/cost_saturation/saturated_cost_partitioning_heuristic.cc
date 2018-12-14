#include "abstraction.h"
#include "cost_partitioning_heuristic.h"
#include "cost_partitioning_collection_generator.h"
#include "max_cost_partitioning_heuristic.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../utils/logging.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

using namespace std;

namespace cost_saturation {
static CostPartitioningHeuristic compute_saturated_cost_partitioning(
    const Abstractions &abstractions,
    const vector<int> &order,
    const vector<int> &costs) {
    assert(abstractions.size() == order.size());
    CostPartitioningHeuristic cp_heuristic;
    vector<int> remaining_costs = costs;
    for (int pos : order) {
        const Abstraction &abstraction = *abstractions[pos];
        vector<int> h_values = abstraction.compute_goal_distances(
            remaining_costs);
        vector<int> saturated_costs = abstraction.compute_saturated_costs(
            h_values, costs.size());
        cp_heuristic.add_h_values(pos, move(h_values));
        reduce_costs(remaining_costs, saturated_costs);
    }
    return cp_heuristic;
}

void add_order_options_to_parser(OptionParser &parser) {
    parser.add_option<shared_ptr<OrderGenerator>>(
        "orders",
        "order generator",
        "greedy_orders()");
    parser.add_option<int>(
        "max_orders",
        "maximum number of orders",
        "infinity",
        Bounds("0", "infinity"));
    parser.add_option<double>(
        "max_time",
        "maximum time for finding orders",
        "200.0",
        Bounds("0", "infinity"));
    parser.add_option<bool>(
        "diversify",
        "only keep orders that have a higher heuristic value than all previous "
        "orders for any of the samples",
        "true");
    parser.add_option<int>(
        "samples",
        "number of samples for diversification",
        "1000",
        Bounds("1", "infinity"));
    parser.add_option<double>(
        "max_optimization_time",
        "maximum time for optimizing each order with hill climbing",
        "2.0",
        Bounds("0.0", "infinity"));
    utils::add_rng_options(parser);
}

static CostPartitioningCollectionGenerator get_cp_collection_generator_from_options(
    const options::Options &opts) {
    return CostPartitioningCollectionGenerator(
        opts.get<shared_ptr<OrderGenerator>>("orders"),
        opts.get<int>("max_orders"),
        opts.get<double>("max_time"),
        opts.get<bool>("diversify"),
        opts.get<int>("samples"),
        opts.get<double>("max_optimization_time"),
        utils::parse_rng_from_options(opts));
}

static shared_ptr<Heuristic> get_max_cp_heuristic(
    options::OptionParser &parser, CPFunction cp_function) {
    parser.document_language_support("action costs", "supported");
    parser.document_language_support(
        "conditional effects",
        "not supported (the heuristic supports them in theory, but none of "
        "the currently implemented abstraction generators do)");
    parser.document_language_support(
        "axioms",
        "not supported (the heuristic supports them in theory, but none of "
        "the currently implemented abstraction generators do)");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    parser.add_list_option<shared_ptr<AbstractionGenerator>>(
        "abstraction_generators",
        "available generators are cartesian() and projections()",
        "[projections(hillclimbing(max_time=60, random_seed=0)), "
        "projections(systematic(2)), cartesian()]");
    add_order_options_to_parser(parser);
    Heuristic::add_options_to_parser(parser);

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
    vector<CostPartitioningHeuristic> cp_heuristics =
        get_cp_collection_generator_from_options(opts).generate_cost_partitionings(
            task_proxy, abstractions, costs, cp_function);
    return make_shared<MaxCostPartitioningHeuristic>(
        opts,
        move(abstractions),
        move(cp_heuristics));
}

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Saturated cost partitioning heuristic",
        "Compute the maximum over multiple saturated cost partitioning "
        "heuristics using different orders. Depending on the options, orders "
        "may be greedy, optimized and/or diverse.");
    return get_max_cp_heuristic(parser, compute_saturated_cost_partitioning);
}

static Plugin<Evaluator> _plugin("saturated_cost_partitioning", _parse);
}
