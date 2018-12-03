#include "diversifier.h"

#include "cost_partitioning_heuristic.h"

#include "../utils/collections.h"

#include <cassert>
#include <numeric>

using namespace std;

namespace cost_saturation {
Diversifier::Diversifier(vector<vector<int>> &&abstract_state_ids_by_sample)
    : abstract_state_ids_by_sample(move(abstract_state_ids_by_sample)),
      // Initialize with -1 to ensure that first cost partitioning is diverse.
      portfolio_h_values(this->abstract_state_ids_by_sample.size(), -1) {
}

bool Diversifier::is_diverse(const CostPartitioningHeuristic &cp_heuristic) {
    bool cp_improves_portfolio = false;
    int num_samples = abstract_state_ids_by_sample.size();
    for (int sample_id = 0; sample_id < num_samples; ++sample_id) {
        int cp_h_value = cp_heuristic.compute_heuristic(
            abstract_state_ids_by_sample[sample_id]);
        assert(utils::in_bounds(sample_id, portfolio_h_values));
        int &portfolio_h_value = portfolio_h_values[sample_id];
        if (cp_h_value > portfolio_h_value) {
            cp_improves_portfolio = true;
            portfolio_h_value = cp_h_value;
        }
    }
    return cp_improves_portfolio;
}

int Diversifier::compute_sum_portfolio_h_value_for_samples() const {
    return accumulate(portfolio_h_values.begin(), portfolio_h_values.end(), 0);
}
}
