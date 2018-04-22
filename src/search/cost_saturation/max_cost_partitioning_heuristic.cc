#include "max_cost_partitioning_heuristic.h"

#include "abstraction.h"
#include "cost_partitioned_heuristic.h"
#include "cost_partitioning_collection_generator.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/logging.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

using namespace std;

namespace cost_saturation {
class AbstractionGenerator;
class OrderGenerator;

static int compute_max_h(
    const CPHeuristics &cp_heuristics,
    const vector<int> &local_state_ids) {
    int max_h = 0;
    for (const CostPartitionedHeuristic &cp_heuristic : cp_heuristics) {
        int sum_h = cp_heuristic.compute_heuristic(local_state_ids);
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
    vector<CostPartitionedHeuristic> &&cp_heuristics_)
    : Heuristic(opts),
      abstractions(move(abstractions_)),
      cp_heuristics(move(cp_heuristics_)),
      debug(opts.get<bool>("debug")) {
    int num_abstractions = abstractions.size();

    // Number of lookup tables.
    int num_heuristics = num_abstractions * cp_heuristics.size();
    int num_lookup_tables = 0;
    for (const auto &cp_heuristic: cp_heuristics) {
        num_lookup_tables += cp_heuristic.size();
    }
    utils::Log() << "Stored lookup tables: " << num_lookup_tables << "/"
                 << num_heuristics << " = "
                 << num_lookup_tables / static_cast<double>(num_heuristics) << endl;

    // Total lookup table size.
    int num_stored_values = 0;
    for (const auto &cp_heuristic : cp_heuristics) {
        for (const auto &cp_values : cp_heuristic.get_h_values_by_heuristic()) {
            num_stored_values += cp_values.h_values.size();
        }
    }
    int num_total_values = 0;
    for (const auto &abstraction : abstractions) {
        num_total_values += abstraction->get_num_states();
    }
    num_total_values *= cp_heuristics.size();
    utils::Log() << "Stored values: " << num_stored_values << "/"
                 << num_total_values << " = "
                 << num_stored_values / static_cast<double>(num_total_values) << endl;

    // Number of stored heuristics.
    unordered_set<int> stored_heuristics;
    for (const auto &cp_heuristic : cp_heuristics) {
        for (const auto &cp_h_values : cp_heuristic.get_h_values_by_heuristic()) {
            stored_heuristics.insert(cp_h_values.heuristic_index);
        }
    }
    utils::Log() << "Stored heuristics: " << stored_heuristics.size() << "/"
                 << abstractions.size() << " = "
                 << static_cast<double>(stored_heuristics.size()) /
        static_cast<double>(num_abstractions) << endl;

    for (int i = 0; i < num_abstractions; ++i) {
        if (!stored_heuristics.count(i)) {
            abstractions[i] = nullptr;
        }
    }
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

int MaxCostPartitioningHeuristic::compute_heuristic(const State &state) {
    vector<int> local_state_ids = get_local_state_ids(abstractions, state);
    int max_h = compute_max_h(cp_heuristics, local_state_ids);
    if (max_h == INF) {
        return DEAD_END;
    }
    return max_h;
}

void add_cost_partitioning_collection_options_to_parser(OptionParser &parser) {
    parser.add_option<int>(
        "max_orders",
        "maximum number of abstraction orders",
        "infinity",
        Bounds("0", "infinity"));
    parser.add_option<double>(
        "max_time",
        "maximum time for finding cost partitionings",
        "10",
        Bounds("0", "infinity"));
    parser.add_option<bool>(
        "diversify",
        "keep orders that improve the portfolio's heuristic value for any of the samples",
        "true");
    parser.add_option<int>(
        "samples",
        "number of samples for diversification",
        "1000",
        Bounds("1", "infinity"));
    parser.add_option<double>(
        "max_optimization_time",
        "maximum time for optimizing",
        "0.0",
        Bounds("0.0", "infinity"));
    parser.add_option<bool>(
        "steepest_ascent",
        "do steepest-ascent hill climbing instead of selecting the first improving successor",
        "false");
    parser.add_option<bool>(
        "sparse",
        "don't store h-value vectors that only contain zeros",
        "true");
    utils::add_rng_options(parser);
}

void prepare_parser_for_cost_partitioning_heuristic(
    options::OptionParser &parser) {
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
    parser.document_property(
        "consistent",
        "yes, if all abstraction generators represent consistent heuristics");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    parser.add_list_option<shared_ptr<AbstractionGenerator>>(
        "abstraction_generators",
        "methods that generate abstractions");
    parser.add_option<shared_ptr<OrderGenerator>>(
        "orders",
        "order generator",
        OptionParser::NONE);
    parser.add_option<bool>(
        "debug",
        "print debugging information",
        "false");
    Heuristic::add_options_to_parser(parser);
}

CostPartitioningCollectionGenerator get_cp_collection_generator_from_options(
    const options::Options &opts) {
    return CostPartitioningCollectionGenerator(
        opts.get<shared_ptr<OrderGenerator>>("orders"),
        opts.get<bool>("sparse"),
        opts.get<int>("max_orders"),
        opts.get<double>("max_time"),
        opts.get<bool>("diversify"),
        opts.get<int>("samples"),
        opts.get<double>("max_optimization_time"),
        opts.get<bool>("steepest_ascent"),
        utils::parse_rng_from_options(opts));
}
}
