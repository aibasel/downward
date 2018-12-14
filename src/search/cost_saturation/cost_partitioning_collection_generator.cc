#include "cost_partitioning_collection_generator.h"

#include "cost_partitioning_heuristic.h"
#include "diversifier.h"
#include "order_generator.h"
#include "order_optimizer.h"
#include "utils.h"

#include "../task_proxy.h"

#include "../task_utils/sampling.h"
#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/countdown_timer.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include <cassert>

using namespace std;

namespace cost_saturation {
static vector<vector<int>> sample_states_and_return_abstract_state_ids(
    const TaskProxy &task_proxy,
    const Abstractions &abstractions,
    sampling::RandomWalkSampler &sampler,
    int num_samples,
    int init_h,
    const DeadEndDetector &is_dead_end,
    double max_sampling_time) {
    assert(num_samples >= 1);
    utils::CountdownTimer sampling_timer(max_sampling_time);
    utils::Log() << "Start sampling" << endl;
    vector<vector<int>> abstract_state_ids_by_sample;
    abstract_state_ids_by_sample.push_back(
        get_abstract_state_ids(abstractions, task_proxy.get_initial_state()));
    while (static_cast<int>(abstract_state_ids_by_sample.size()) < num_samples
           && !sampling_timer.is_expired()) {
        abstract_state_ids_by_sample.push_back(
            get_abstract_state_ids(abstractions, sampler.sample_state(init_h, is_dead_end)));
    }
    utils::Log() << "Samples: " << abstract_state_ids_by_sample.size() << endl;
    utils::Log() << "Sampling time: " << sampling_timer.get_elapsed_time() << endl;
    return abstract_state_ids_by_sample;
}

CostPartitioningCollectionGenerator::CostPartitioningCollectionGenerator(
    const shared_ptr<OrderGenerator> &cp_generator,
    int max_orders,
    double max_time,
    bool diversify,
    int num_samples,
    double max_optimization_time,
    const shared_ptr<utils::RandomNumberGenerator> &rng)
    : cp_generator(cp_generator),
      max_orders(max_orders),
      max_time(max_time),
      diversify(diversify),
      num_samples(num_samples),
      max_optimization_time(max_optimization_time),
      rng(rng) {
}

CostPartitioningCollectionGenerator::~CostPartitioningCollectionGenerator() {
}

vector<CostPartitioningHeuristic>
CostPartitioningCollectionGenerator::generate_cost_partitionings(
    const TaskProxy &task_proxy,
    const Abstractions &abstractions,
    const vector<int> &costs,
    CPFunction cp_function) const {
    utils::Log log;
    State initial_state = task_proxy.get_initial_state();
    vector<int> abstract_state_ids_for_init = get_abstract_state_ids(
        abstractions, initial_state);

    // If any abstraction detects unsolvability in the initial state, we only
    // need a single order (any order suffices).
    CostPartitioningHeuristic default_order_cp = cp_function(
        abstractions, get_default_order(abstractions.size()), costs);
    if (default_order_cp.compute_heuristic(abstract_state_ids_for_init) == INF) {
        return {
                   default_order_cp
        };
    }

    cp_generator->initialize(abstractions, costs);

    // Compute cost partitioning heuristic for sampling.
    Order order = cp_generator->compute_order_for_state(
        abstractions, costs, abstract_state_ids_for_init, false);
    CostPartitioningHeuristic cp_for_sampling = cp_function(
        abstractions, order, costs);
    function<int (const State &state)> sampling_heuristic =
        [&abstractions, &cp_for_sampling](const State &state) {
            return cp_for_sampling.compute_heuristic(
                get_abstract_state_ids(abstractions, state));
        };

    int init_h = sampling_heuristic(initial_state);

    // Compute dead end detector which uses the sampling heuristic.
    DeadEndDetector is_dead_end = [&sampling_heuristic](const State &state) {
            return sampling_heuristic(state) == INF;
        };
    sampling::RandomWalkSampler sampler(task_proxy, *rng);

    utils::CountdownTimer timer(max_time);
    unique_ptr<Diversifier> diversifier;
    if (diversify) {
        double max_sampling_time = timer.get_remaining_time();
        diversifier = utils::make_unique_ptr<Diversifier>(
            sample_states_and_return_abstract_state_ids(
                task_proxy, abstractions, sampler, num_samples, init_h, is_dead_end, max_sampling_time));
    }

    vector<CostPartitioningHeuristic> cp_heuristics;
    int evaluated_orders = 0;
    log << "Start computing cost partitionings" << endl;
    while (static_cast<int>(cp_heuristics.size()) < max_orders &&
           (!timer.is_expired() || cp_heuristics.empty())) {
        // Use initial state as first sample.
        State sample = (evaluated_orders == 0)
            ? initial_state
            : sampler.sample_state(init_h, is_dead_end);
        assert(!is_dead_end(sample));
        // If sampling took too long and we already found a cost partitioning,
        // abort the loop.
        if (timer.is_expired() && !cp_heuristics.empty()) {
            break;
        }
        vector<int> abstract_state_ids = get_abstract_state_ids(abstractions, sample);

        // Only be verbose for first sample.
        bool verbose = (evaluated_orders == 0);

        // Find order and compute cost partitioning for it.
        Order order = cp_generator->compute_order_for_state(
            abstractions, costs, abstract_state_ids, verbose);
        CostPartitioningHeuristic cp_heuristic = cp_function(
            abstractions, order, costs);

        // Optimize order.
        if (max_optimization_time > 0) {
            utils::CountdownTimer timer(max_optimization_time);
            int incumbent_h_value = cp_heuristic.compute_heuristic(abstract_state_ids);
            do_hill_climbing(
                cp_function, timer, abstractions, costs, abstract_state_ids, order,
                cp_heuristic, incumbent_h_value, verbose);
            if (verbose) {
                log << "Time for optimizing order: " << timer.get_elapsed_time()
                    << endl;
            }
        }

        // If diversify=true, only add order if it improves upon previously
        // added orders.
        if (!diversifier || diversifier->is_diverse(cp_heuristic)) {
            cp_heuristics.push_back(move(cp_heuristic));
            if (diversify) {
                log << "Sum over max h values for " << num_samples
                    << " samples after " << timer.get_elapsed_time()
                    << " of diversification: "
                    << diversifier->compute_sum_portfolio_h_value_for_samples()
                    << endl;
            }
        }

        ++evaluated_orders;
    }
    log << "Cost partitionings: " << cp_heuristics.size() << endl;
    log << "Time for computing cost partitionings: " << timer.get_elapsed_time()
        << endl;
    return cp_heuristics;
}
}
