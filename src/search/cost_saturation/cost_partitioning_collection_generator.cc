#include "cost_partitioning_collection_generator.h"

#include "abstraction.h"
#include "cost_partitioned_heuristic.h"
#include "diversifier.h"
#include "order_generator.h"
#include "order_optimizer.h"
#include "utils.h"

#include "../task_proxy.h"

#include "../task_utils/sampling.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/countdown_timer.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include <cassert>

using namespace std;

namespace cost_saturation {
static vector<vector<int>> sample_states_and_return_local_state_ids(
    const TaskProxy &task_proxy,
    const vector<unique_ptr<Abstraction>> &abstractions,
    sampling::RandomWalkSampler &sampler,
    int num_samples) {
    assert(num_samples >= 1);
    utils::Timer sampling_timer;
    utils::Log() << "Start sampling" << endl;
    vector<vector<int>> local_state_ids_by_sample;
    local_state_ids_by_sample.push_back(
        get_local_state_ids(abstractions, task_proxy.get_initial_state()));
    while (static_cast<int>(local_state_ids_by_sample.size()) < num_samples) {
        local_state_ids_by_sample.push_back(
            get_local_state_ids(abstractions, sampler.sample_state()));
    }
    utils::Log() << "Samples: " << local_state_ids_by_sample.size() << endl;
    utils::Log() << "Sampling time: " << sampling_timer << endl;
    return local_state_ids_by_sample;
}

CostPartitioningCollectionGenerator::CostPartitioningCollectionGenerator(
    const shared_ptr<OrderGenerator> &cp_generator,
    bool sparse,
    int max_orders,
    double max_time,
    bool diversify,
    int num_samples,
    double max_optimization_time,
    bool steepest_ascent,
    const shared_ptr<utils::RandomNumberGenerator> &rng)
    : cp_generator(cp_generator),
      sparse(sparse),
      max_orders(max_orders),
      max_time(max_time),
      diversify(diversify),
      num_samples(num_samples),
      max_optimization_time(max_optimization_time),
      steepest_ascent(steepest_ascent),
      rng(rng) {
}

CostPartitioningCollectionGenerator::~CostPartitioningCollectionGenerator() {
}

vector<CostPartitionedHeuristic> CostPartitioningCollectionGenerator::get_cost_partitionings(
    const TaskProxy &task_proxy,
    const Abstractions &abstractions,
    const vector<int> &costs,
    CPFunction cp_function) const {
    State initial_state = task_proxy.get_initial_state();
    vector<int> local_state_ids = get_local_state_ids(abstractions, initial_state);

    CostPartitionedHeuristic default_order_cp = cp_function(
        abstractions, get_default_order(abstractions.size()), costs, true);
    if (default_order_cp.compute_heuristic(local_state_ids) == INF) {
        return {
                   default_order_cp
        };
    }

    cp_generator->initialize(task_proxy, abstractions, costs);

    Order order = cp_generator->get_next_order(
        task_proxy, abstractions, costs, local_state_ids, false);
    CostPartitionedHeuristic cp_for_sampling = cp_function(abstractions, order, costs, true);
    function<int (const State &state)> sampling_heuristic =
        [&abstractions, &cp_for_sampling](const State &state) {
            return cp_for_sampling.compute_heuristic(abstractions, state);
        };

    int init_h = sampling_heuristic(initial_state);
    DeadEndDetector is_dead_end = [&sampling_heuristic](const State &state) {
                                      return sampling_heuristic(state) == INF;
                                  };
    sampling::RandomWalkSampler sampler(task_proxy, init_h, rng, is_dead_end);

    unique_ptr<Diversifier> diversifier;
    if (diversify) {
        diversifier = utils::make_unique_ptr<Diversifier>(
            sample_states_and_return_local_state_ids(
                task_proxy, abstractions, sampler, num_samples));
    }

    vector<CostPartitionedHeuristic> cp_heuristics;
    utils::CountdownTimer timer(max_time);
    int evaluated_orders = 0;
    int peak_memory_without_cps = utils::get_peak_memory_in_kb();
    utils::Log() << "Start computing cost partitionings" << endl;
    while (static_cast<int>(cp_heuristics.size()) < max_orders &&
           !timer.is_expired() && cp_generator->has_next_cost_partitioning()) {
        State sample = evaluated_orders == 0 ? initial_state : sampler.sample_state();
        assert(!is_dead_end(sample));
        if (timer.is_expired() && !cp_heuristics.empty()) {
            break;
        }
        vector<int> local_state_ids = get_local_state_ids(abstractions, sample);
        // Only be verbose for first sample.
        bool verbose = (evaluated_orders == 0);
        Order order = cp_generator->get_next_order(
            task_proxy, abstractions, costs, local_state_ids, verbose);
        CostPartitionedHeuristic cp_heuristic = cp_function(
            abstractions, order, costs, sparse);

        if (max_optimization_time > 0) {
            utils::CountdownTimer timer(max_optimization_time);
            int incumbent_h_value = cp_heuristic.compute_heuristic(local_state_ids);
            do_hill_climbing(
                cp_function, timer, abstractions, costs, local_state_ids, order,
                cp_heuristic, incumbent_h_value, steepest_ascent, sparse, verbose);
            if (verbose) {
                utils::Log() << "Time for optimizing order: " << timer.get_elapsed_time() << endl;
                utils::Log() << "Time for optimizing order has expired: " << timer.is_expired() << endl;
            }
        }

        if (!diversify || (diversifier->is_diverse(cp_heuristic))) {
            cp_heuristics.push_back(move(cp_heuristic));
        }

        ++evaluated_orders;
    }
    int peak_memory_with_cps = utils::get_peak_memory_in_kb();
    utils::Log() << "Orders: " << cp_heuristics.size() << endl;
    utils::Log() << "Time for computing cost partitionings: " << timer.get_elapsed_time() << endl;
    utils::Log() << "Memory for cost partitionings: "
                 << peak_memory_with_cps - peak_memory_without_cps << " KB" << endl;
    return cp_heuristics;
}
}
