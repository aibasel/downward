#include "order_generator_dynamic_greedy.h"

#include "abstraction.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../utils/collections.h"
#include "../utils/logging.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <unordered_set>

using namespace std;

namespace cost_saturation {
OrderGeneratorDynamicGreedy::OrderGeneratorDynamicGreedy(const Options &opts)
    : scoring_function(
        static_cast<ScoringFunction>(opts.get_enum("scoring_function"))) {
}

Order OrderGeneratorDynamicGreedy::compute_dynamic_greedy_order_for_sample(
    const Abstractions &abstractions,
    const vector<int> &local_state_ids,
    vector<int> remaining_costs) const {
    assert(abstractions.size() == local_state_ids.size());
    Order order;

    vector<int> remaining_abstractions = get_default_order(abstractions.size());

    while (!remaining_abstractions.empty()) {
        int num_remaining = remaining_abstractions.size();
        vector<int> current_h_values;
        vector<vector<int>> current_saturated_costs;
        current_h_values.reserve(num_remaining);
        current_saturated_costs.reserve(num_remaining);

        vector<int> saturated_costs_for_best_abstraction;
        for (int abs_id : remaining_abstractions) {
            assert(utils::in_bounds(abs_id, local_state_ids));
            int local_state_id = local_state_ids[abs_id];
            Abstraction &abstraction = *abstractions[abs_id];
            auto pair = abstraction.compute_goal_distances_and_saturated_costs(
                remaining_costs);
            vector<int> &h_values = pair.first;
            vector<int> &saturated_costs = pair.second;
            assert(utils::in_bounds(local_state_id, h_values));
            int h = h_values[local_state_id];
            current_h_values.push_back(h);
            current_saturated_costs.push_back(move(saturated_costs));
        }

        vector<int> surplus_costs = compute_all_surplus_costs(
            remaining_costs, current_saturated_costs);

        double highest_score = -numeric_limits<double>::max();
        int best_rem_id = -1;
        for (int rem_id = 0; rem_id < num_remaining; ++rem_id) {
            int used_costs = compute_costs_stolen_by_heuristic(
                current_saturated_costs[rem_id], surplus_costs);
            double score = compute_score(
                current_h_values[rem_id], used_costs, scoring_function);
            if (score > highest_score) {
                best_rem_id = rem_id;
                highest_score = score;
            }
        }
        order.push_back(remaining_abstractions[best_rem_id]);
        reduce_costs(remaining_costs, current_saturated_costs[best_rem_id]);
        utils::swap_and_pop_from_vector(remaining_abstractions, best_rem_id);
    }
    return order;
}

void OrderGeneratorDynamicGreedy::initialize(
    const TaskProxy &,
    const Abstractions &,
    const vector<int> &) {
    utils::Log() << "Initialize dynamic greedy order generator" << endl;
}

Order OrderGeneratorDynamicGreedy::get_next_order(
    const TaskProxy &,
    const Abstractions &abstractions,
    const vector<int> &costs,
    const vector<int> &local_state_ids,
    bool verbose) {
    utils::Timer greedy_timer;
    vector<int> order = compute_dynamic_greedy_order_for_sample(
        abstractions, local_state_ids, costs);

    if (verbose) {
        utils::Log() << "Time for computing dynamic greedy order: "
                     << greedy_timer << endl;
    }

    assert(order.size() == abstractions.size());
    return order;
}


static shared_ptr<OrderGenerator> _parse_greedy(OptionParser &parser) {
    add_scoring_function_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<OrderGeneratorDynamicGreedy>(opts);
}

static PluginShared<OrderGenerator> _plugin_greedy(
    "dynamic_greedy_orders", _parse_greedy);
}
