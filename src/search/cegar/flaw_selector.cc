#include "flaw_selector.h"

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
FlawSelector::FlawSelector(TaskProxy task_proxy, PickFlaw pick)
    : task_proxy(task_proxy),
      pick(pick) {
    if (pick == PickFlaw::MIN_HADD || pick == PickFlaw::MAX_HADD)
        additive_heuristic = get_additive_heuristic(task_proxy);
}

int FlawSelector::get_constrainedness(const AbstractState &state, int var_id) const {
    int rem_values = state.count(var_id);
    assert(rem_values >= 2);
    return rem_values;
}

double FlawSelector::get_refinedness(const AbstractState &state, int var_id) const {
    double all_values = task_proxy.get_variables()[var_id].get_domain_size();
    assert(all_values >= 2);
    double rem_values = state.count(var_id);
    assert(2 <= rem_values && rem_values <= all_values);
    double refinedness = -(rem_values / all_values);
    assert(-1.0 <= refinedness && refinedness < 0.0);
    return refinedness;
}

int FlawSelector::get_hadd_value(int var_id, int value) const {
    assert(additive_heuristic);
    int hadd = additive_heuristic->get_cost(var_id, value);
    // TODO: Can this still happen?
    if (hadd == -1)
        // Fact is unreachable.
        return INF - 1;
    return hadd;
}

int FlawSelector::get_extreme_hadd_value(int var_id, const vector<int> &values) const {
    assert(pick == PickFlaw::MIN_HADD || pick == PickFlaw::MAX_HADD);
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
    if (pick == PickFlaw::MIN_HADD) {
        return min_hadd;
    } else {
        return max_hadd;
    }
}

double FlawSelector::rate_flaw(const AbstractState &state, const Flaw &flaw) const {
    int var_id = flaw.first;
    const vector<int> &values = flaw.second;
    double rating;
    if (pick == PickFlaw::MIN_CONSTRAINED || pick == PickFlaw::MAX_CONSTRAINED) {
        rating = get_constrainedness(state, var_id);
    } else if (pick == PickFlaw::MIN_REFINED || pick == PickFlaw::MAX_REFINED) {
        rating = get_refinedness(state, var_id);
    } else if (pick == PickFlaw::MIN_HADD || pick == PickFlaw::MAX_HADD) {
        rating  = get_extreme_hadd_value(var_id, values);
    } else {
        cout << "Invalid pick strategy: " << static_cast<int>(pick) << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
    if (pick == PickFlaw::MIN_CONSTRAINED || pick == PickFlaw::MIN_REFINED ||
        pick == PickFlaw::MIN_HADD) {
        rating = -rating;
    }
    return rating;
}

const Flaw &FlawSelector::pick_flaw(const AbstractState &state, const Flaws &flaws) const {
    assert(!flaws.empty());

    // Use shortcut if there is only one flaw.
    if (flaws.size() == 1) {
        return flaws[0];
    }

    if (DEBUG) {
        cout << "Flaws in state " << state << ": " << endl;
        for (const Flaw &flaw : flaws)
            cout << flaw.first << "=" << flaw.second << " ";
        cout << endl;
    }

    if (pick == PickFlaw::RANDOM) {
        return *g_rng.choose(flaws);
    }

    double max_rating = numeric_limits<double>::lowest();
    const Flaw *selected_flaw = nullptr;
    for (const Flaw &flaw : flaws) {
        double rating = rate_flaw(state, flaw);
        if (rating > max_rating) {
            selected_flaw = &flaw;
            max_rating = rating;
        }
    }
    assert(selected_flaw);
    return *selected_flaw;
}
}
