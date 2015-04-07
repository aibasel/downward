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
    else
        assert(!additive_heuristic);
}

// TODO: Turn into classes.
int FlawSelector::pick_split_index(const AbstractState &state, const Splits &splits) const {
    // TODO: Return reference to split instead of index and rename method.
    assert(!splits.empty());
    // Shortcut for condition lists with only one element.
    if (splits.size() == 1) {
        return 0;
    }
    if (DEBUG) {
        cout << "Split: " << state.str() << endl;
        for (size_t i = 0; i < splits.size(); ++i) {
            const Split &split = splits[i];
            int var = split.first;
            const vector<int> &wanted = split.second;
            if (DEBUG)
                cout << var << "=" << wanted << " ";
        }
    }
    if (DEBUG)
        cout << endl;
    int cond = -1;
    int random_cond = g_rng(splits.size());
    if (pick == RANDOM) {
        cond = random_cond;
    } else if (pick == MIN_CONSTRAINED || pick == MAX_CONSTRAINED) {
        int max_rest = -1;
        int min_rest = INF;
        for (size_t i = 0; i < splits.size(); ++i) {
            int rest = state.count(splits[i].first);
            assert(rest >= 2);
            if (rest > max_rest && pick == MIN_CONSTRAINED) {
                cond = i;
                max_rest = rest;
            }
            if (rest < min_rest && pick == MAX_CONSTRAINED) {
                cond = i;
                min_rest = rest;
            }
        }
    } else if (pick == MIN_REFINED || pick == MAX_REFINED) {
        double min_refinement = 0.0;
        double max_refinement = -1.1;
        int i = 0;
        for (auto split : splits) {
            int var_id = split.first;
            double all_values = task_proxy.get_variables()[var_id].get_domain_size();
            double rest = state.count(var_id);
            assert(all_values >= 2);
            assert(rest >= 2);
            assert(rest <= all_values);
            double refinement = -(rest / all_values);
            assert(refinement >= -1.0);
            assert(refinement < 0.0);
            if (refinement < min_refinement && pick == MIN_REFINED) {
                cond = i;
                min_refinement = refinement;
            }
            if (refinement > max_refinement && pick == MAX_REFINED) {
                cond = i;
                max_refinement = refinement;
            }
            ++i;
        }
    } else if (pick == MIN_HADD || pick == MAX_HADD) {
        int min_hadd = INF;
        int max_hadd = -2;
        for (size_t i = 0; i < splits.size(); ++i) {
            int var = splits[i].first;
            const vector<int> &values = splits[i].second;
            for (size_t j = 0; j < values.size(); ++j) {
                int value = values[j];
                int hadd_value = additive_heuristic->get_cost(var, value);
                if (hadd_value == -1 && pick == MIN_HADD) {
                    // Fact is unreachable --> Choose it last.
                    hadd_value = INF - 1;
                }
                if (pick == MIN_HADD && hadd_value < min_hadd) {
                    cond = i;
                    min_hadd = hadd_value;
                } else if (pick == MAX_HADD && hadd_value > max_hadd) {
                    cond = i;
                    max_hadd = hadd_value;
                }
            }
        }
    } else {
        cout << "Invalid pick strategy: " << pick << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
    assert(in_bounds(cond, splits));
    return cond;
}
}
