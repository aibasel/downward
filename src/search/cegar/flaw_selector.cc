#include "flaw_selector.h"

#include "abstract_state.h"

#include "../additive_heuristic.h"
#include "../globals.h"
#include "../rng.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

using namespace std;

namespace cegar {

FlawSelector::FlawSelector(TaskProxy task_proxy, PickStrategy pick)
    : task_proxy(task_proxy),
      pick(pick) {
    cout << "Use flaw-selection strategy " << pick << endl;
    if (pick == MIN_HADD || pick == MAX_HADD)
        additive_heuristic = get_additive_heuristic(task_proxy);
}

int FlawSelector::get_constrainedness(const AbstractState &state, const Flaw &flaw) const {
    int var_id = flaw.first;
    int num_remaining_values = state.count(var_id);
    assert(num_remaining_values >= 2);
    return num_remaining_values;
}

double FlawSelector::get_refinedness(const AbstractState &state, const Flaw &flaw) const {
    int var_id = flaw.first;
    double all_values = task_proxy.get_variables()[var_id].get_domain_size();
    double rest = state.count(var_id);
    assert(all_values >= 2);
    assert(rest >= 2);
    assert(rest <= all_values);
    double refinedness = -(rest / all_values);
    assert(refinedness >= -1.0);
    assert(refinedness < 0.0);
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
    assert(pick == MIN_HADD || pick == MAX_HADD);
    int min_hadd = INF;
    int max_hadd = -2;
    for (int value : values) {
        int hadd = get_hadd_value(var_id, value);
        if (pick == MIN_HADD && hadd < min_hadd) {
            min_hadd = hadd;
        } else if (pick == MAX_HADD && hadd > max_hadd) {
            max_hadd = hadd;
        }
    }
    if (pick == MIN_HADD)
        return min_hadd;
    else
        return max_hadd;
}

int FlawSelector::pick_flaw_index(const AbstractState &state, const Flaws &flaws) const {
    // TODO: Return reference to flaw instead of index and rename method.
    assert(!flaws.empty());
    // Shortcut for condition lists with only one element.
    if (flaws.size() == 1) {
        return 0;
    }
    if (DEBUG) {
        cout << "Flaw: " << state.str() << endl;
        for (const Flaw &flaw : flaws) {
            int var = flaw.first;
            const vector<int> &wanted = flaw.second;
            if (DEBUG)
                cout << var << "=" << wanted << " ";
        }
    }
    if (DEBUG)
        cout << endl;
    int cond = -1;
    int random_cond = g_rng(flaws.size());
    if (pick == RANDOM) {
        cond = random_cond;
    } else if (pick == MIN_CONSTRAINED || pick == MAX_CONSTRAINED) {
        int max_rest = -1;
        int min_rest = INF;
        int i = 0;
        for (const Flaw &flaw : flaws) {
            int value = get_constrainedness(state, flaw);
            if (value > max_rest && pick == MIN_CONSTRAINED) {
                cond = i;
                max_rest = value;
            }
            if (value < min_rest && pick == MAX_CONSTRAINED) {
                cond = i;
                min_rest = value;
            }
            ++i;
        }
    } else if (pick == MIN_REFINED || pick == MAX_REFINED) {
        double min_refinement = 0.0;
        double max_refinement = -1.1;
        int i = 0;
        for (auto flaw : flaws) {
            double refinedness = get_refinedness(state, flaw);
            if (refinedness < min_refinement && pick == MIN_REFINED) {
                cond = i;
                min_refinement = refinedness;
            }
            if (refinedness > max_refinement && pick == MAX_REFINED) {
                cond = i;
                max_refinement = refinedness;
            }
            ++i;
        }
    } else if (pick == MIN_HADD || pick == MAX_HADD) {
        int min_hadd = INF;
        int max_hadd = -2;
        int i = 0;
        for (const Flaw &flaw : flaws) {
            int var_id = flaw.first;
            const vector<int> &values = flaw.second;
            for (int value : values) {
                int result = get_hadd_value(var_id, value);
                if (pick == MIN_HADD && result < min_hadd) {
                    cond = i;
                    min_hadd = result;
                } else if (pick == MAX_HADD && result > max_hadd) {
                    cond = i;
                    max_hadd = result;
                }
            }
            ++i;
        }
    } else {
        cout << "Invalid pick strategy: " << pick << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
    assert(in_bounds(cond, flaws));
    return cond;
}
}
