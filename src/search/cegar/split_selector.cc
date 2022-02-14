#include "split_selector.h"

#include "abstract_state.h"
#include "utils.h"

#include "../heuristics/additive_heuristic.h"

#include "../utils/logging.h"
#include "../utils/rng.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace std;

namespace cegar {
SplitSelector::SplitSelector(
    const shared_ptr<AbstractTask> &task,
    PickSplit pick)
    : task(task),
      task_proxy(*task),
      pick(pick) {
    if (pick == PickSplit::MIN_HADD || pick == PickSplit::MAX_HADD) {
        additive_heuristic = create_additive_heuristic(task);
        additive_heuristic->compute_heuristic_for_cegar(
            task_proxy.get_initial_state());
    }
}

// Define here to avoid include in header.
SplitSelector::~SplitSelector() {
}

int SplitSelector::get_num_unwanted_values(
    const AbstractState &state, const Split &split) const {
    int num_unwanted_values = state.count(split.var_id) - split.values.size();
    assert(num_unwanted_values >= 1);
    return num_unwanted_values;
}

double SplitSelector::get_refinedness(const AbstractState &state, int var_id) const {
    double all_values = task_proxy.get_variables()[var_id].get_domain_size();
    assert(all_values >= 2);
    double remaining_values = state.count(var_id);
    assert(2 <= remaining_values && remaining_values <= all_values);
    double refinedness = -(remaining_values / all_values);
    assert(-1.0 <= refinedness && refinedness < 0.0);
    return refinedness;
}

int SplitSelector::get_hadd_value(int var_id, int value) const {
    assert(additive_heuristic);
    int hadd = additive_heuristic->get_cost_for_cegar(var_id, value);
    assert(hadd != -1);
    return hadd;
}

int SplitSelector::get_min_hadd_value(int var_id, const vector<int> &values) const {
    int min_hadd = numeric_limits<int>::max();
    for (int value : values) {
        const int hadd = get_hadd_value(var_id, value);
        if (hadd < min_hadd) {
            min_hadd = hadd;
        }
    }
    return min_hadd;
}

int SplitSelector::get_max_hadd_value(int var_id, const vector<int> &values) const {
    int max_hadd = -1;
    for (int value : values) {
        const int hadd = get_hadd_value(var_id, value);
        if (hadd > max_hadd) {
            max_hadd = hadd;
        }
    }
    return max_hadd;
}

double SplitSelector::rate_split(const AbstractState &state, const Split &split) const {
    int var_id = split.var_id;
    const vector<int> &values = split.values;
    double rating;
    switch (pick) {
    case PickSplit::MIN_UNWANTED:
        rating = -get_num_unwanted_values(state, split);
        break;
    case PickSplit::MAX_UNWANTED:
        rating = get_num_unwanted_values(state, split);
        break;
    case PickSplit::MIN_REFINED:
        rating = -get_refinedness(state, var_id);
        break;
    case PickSplit::MAX_REFINED:
        rating = get_refinedness(state, var_id);
        break;
    case PickSplit::MIN_HADD:
        rating = -get_min_hadd_value(var_id, values);
        break;
    case PickSplit::MAX_HADD:
        rating = get_max_hadd_value(var_id, values);
        break;
    default:
        cerr << "Invalid pick strategy: " << static_cast<int>(pick) << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
    return rating;
}

const Split &SplitSelector::pick_split(const AbstractState &state,
                                       const vector<Split> &splits,
                                       utils::RandomNumberGenerator &rng) const {
    assert(!splits.empty());

    if (splits.size() == 1) {
        return splits[0];
    }

    if (pick == PickSplit::RANDOM) {
        return *rng.choose(splits);
    }

    double max_rating = numeric_limits<double>::lowest();
    const Split *selected_split = nullptr;
    for (const Split &split : splits) {
        double rating = rate_split(state, split);
        if (rating > max_rating) {
            selected_split = &split;
            max_rating = rating;
        }
    }
    assert(selected_split);
    return *selected_split;
}
}
