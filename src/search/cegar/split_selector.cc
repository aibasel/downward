#include "split_selector.h"

#include "abstract_state.h"

#include "../additive_heuristic.h"
#include "../globals.h"
#include "../rng.h"
#include "../utilities.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace std;

namespace cegar {
SplitSelector::SplitSelector(std::shared_ptr<AbstractTask> task, PickSplit pick)
    : task(task),
      task_proxy(*task),
      pick(pick) {
    if (pick == PickSplit::MIN_HADD || pick == PickSplit::MAX_HADD)
        additive_heuristic = get_additive_heuristic(task);
}

int SplitSelector::get_num_unwanted_values(const AbstractState &state,
                                           const Split &split) const {
    int num_unwanted_values = state.count(split.var_id) - split.values.size();
    assert(num_unwanted_values >= 1);
    return num_unwanted_values;
}

double SplitSelector::get_refinedness(const AbstractState &state, int var_id) const {
    double all_values = task_proxy.get_variables()[var_id].get_domain_size();
    assert(all_values >= 2);
    double rem_values = state.count(var_id);
    assert(2 <= rem_values && rem_values <= all_values);
    double refinedness = -(rem_values / all_values);
    assert(-1.0 <= refinedness && refinedness < 0.0);
    return refinedness;
}

int SplitSelector::get_hadd_value(int var_id, int value) const {
    assert(additive_heuristic);
    int hadd = additive_heuristic->get_cost(var_id, value);
    assert(hadd != -1);
    return hadd;
}

int SplitSelector::get_extreme_hadd_value(int var_id, const vector<int> &values) const {
    assert(pick == PickSplit::MIN_HADD || pick == PickSplit::MAX_HADD);
    int min_hadd = numeric_limits<int>::max();
    int max_hadd = -1;
    for (int value : values) {
        int hadd = get_hadd_value(var_id, value);
        if (hadd < min_hadd) {
            min_hadd = hadd;
        }
        if (hadd > max_hadd) {
            max_hadd = hadd;
        }
    }
    if (pick == PickSplit::MIN_HADD) {
        return min_hadd;
    } else {
        return max_hadd;
    }
}

double SplitSelector::rate_split(const AbstractState &state, const Split &split) const {
    int var_id = split.var_id;
    const vector<int> &values = split.values;
    double rating;
    if (pick == PickSplit::MIN_UNWANTED || pick == PickSplit::MAX_UNWANTED) {
        rating = get_num_unwanted_values(state, split);
    } else if (pick == PickSplit::MIN_REFINED || pick == PickSplit::MAX_REFINED) {
        rating = get_refinedness(state, var_id);
    } else if (pick == PickSplit::MIN_HADD || pick == PickSplit::MAX_HADD) {
        rating = get_extreme_hadd_value(var_id, values);
    } else {
        cout << "Invalid pick strategy: " << static_cast<int>(pick) << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
    if (pick == PickSplit::MIN_UNWANTED || pick == PickSplit::MIN_REFINED ||
        pick == PickSplit::MIN_HADD) {
        rating = -rating;
    }
    return rating;
}

const Split &SplitSelector::pick_split(const AbstractState &state,
                                       const vector<Split> &splits) const {
    assert(!splits.empty());

    if (splits.size() == 1) {
        return splits[0];
    }

    if (DEBUG) {
        cout << "Splits for state " << state << ": " << endl;
        for (const Split &split : splits)
            cout << split.var_id << "=" << split.values << " ";
        cout << endl;
    }

    if (pick == PickSplit::RANDOM) {
        return *g_rng.choose(splits);
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
