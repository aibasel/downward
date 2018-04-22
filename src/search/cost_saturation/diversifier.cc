#include "diversifier.h"

#include "abstraction.h"
#include "cost_partitioned_heuristic.h"
#include "order_generator.h"
#include "utils.h"

#include "../task_proxy.h"

#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/countdown_timer.h"

#include <cassert>
#include <unordered_set>

using namespace std;

namespace cost_saturation {
Diversifier::Diversifier(vector<vector<int>> &&local_state_ids_by_sample)
    : local_state_ids_by_sample(move(local_state_ids_by_sample)) {
    // Initialize portfolio h values with -1 to ensure that first CP is diverse.
    portfolio_h_values.assign(this->local_state_ids_by_sample.size(), -1);
}

bool Diversifier::is_diverse(const CostPartitionedHeuristic &cp) {
    bool cp_improves_portfolio = false;
    for (size_t sample_id = 0; sample_id < local_state_ids_by_sample.size(); ++sample_id) {
        int cp_h_value = cp.compute_heuristic(local_state_ids_by_sample[sample_id]);
        assert(utils::in_bounds(sample_id, portfolio_h_values));
        int &portfolio_h_value = portfolio_h_values[sample_id];
        if (cp_h_value > portfolio_h_value) {
            cp_improves_portfolio = true;
            portfolio_h_value = cp_h_value;
        }
    }

    // Statistics.
    if (cp_improves_portfolio) {
        int sum_portfolio_h = accumulate(
            portfolio_h_values.begin(), portfolio_h_values.end(), 0);
        utils::Log() << "Portfolio sum h value: " << sum_portfolio_h << endl;
    }

    return cp_improves_portfolio;
}
}
