#include "cg_cache.h"

#include "causal_graph.h"
#include "global_state.h"
#include "globals.h"
#include "utilities.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>
using namespace std;

const int CGCache::NOT_COMPUTED;

CGCache::CGCache() {
    cout << "Initializing heuristic cache... " << flush;

    int var_count = g_variable_domain.size();
    const CausalGraph *cg = g_causal_graph;

    // Compute inverted causal graph.
    depends_on.resize(var_count);
    for (int var = 0; var < var_count; ++var) {
        const vector<int> &succ = cg->get_pre_to_eff(var);
        for (size_t i = 0; i < succ.size(); ++i) {
            // Ignore arcs that are not part of the reduced CG:
            // These are ignored by the CG heuristic.
            if (succ[i] > var)
                depends_on[succ[i]].push_back(var);
        }
    }

    // Compute transitive closure of inverted reduced causal graph.
    // This is made easier because it is acyclic and the variables
    // are in topological order.
    for (int var = 0; var < var_count; ++var) {
        int direct_depend_count = depends_on[var].size();
        for (int i = 0; i < direct_depend_count; ++i) {
            int affector = depends_on[var][i];
            assert(affector < var);
            depends_on[var].insert(depends_on[var].end(),
                                   depends_on[affector].begin(),
                                   depends_on[affector].end());
        }
        sort(depends_on[var].begin(), depends_on[var].end());
        depends_on[var].erase(unique(depends_on[var].begin(), depends_on[var].end()),
                              depends_on[var].end());
    }

    const int MAX_CACHE_SIZE = 1000000;

    cache.resize(var_count);
    helpful_transition_cache.resize(var_count);

    for (int var = 0; var < var_count; ++var) {
        int required_cache_size = 1;
        bool can_cache = true;

        int var_domain = g_variable_domain[var];
        if (is_product_within_limit(var_domain, var_domain - 1,
                                    MAX_CACHE_SIZE)) {
            required_cache_size = var_domain * (var_domain - 1);
        } else {
            can_cache = false;
        }

        if (can_cache) {
            for (size_t i = 0; i < depends_on[var].size(); ++i) {
                int relevant_var = depends_on[var][i];
                if (cache[relevant_var].empty()) {
                    /*
                      Check if var depends on a variable var_i that is
                      not cached. This is possible even if var would
                      have an acceptable cache size because the domain
                      of var_i contributes quadratically to its own
                      cache size but only linearly to the cache size
                      of var.
                    */
                    can_cache = false;
                    break;
                }
                int relevant_var_domain = g_variable_domain[relevant_var];
                if (!is_product_within_limit(required_cache_size,
                                             relevant_var_domain,
                                             MAX_CACHE_SIZE)) {
                    can_cache = false;
                    break;
                }
                required_cache_size *= relevant_var_domain;
            }
        }
        if (can_cache) {
            //  cout << "Cache for var " << var << ": "
            //       << required_cache_size << " entries" << endl;
            cache[var].resize(required_cache_size, NOT_COMPUTED);
            helpful_transition_cache[var].resize(required_cache_size, 0);
        }
    }

    cout << "done!" << endl;
}

CGCache::~CGCache() {
}

int CGCache::get_index(int var, const GlobalState &state,
                       int from_val, int to_val) const {
    assert(is_cached(var));
    assert(from_val != to_val);
    int index = from_val;
    int multiplier = g_variable_domain[var];
    for (size_t i = 0; i < depends_on[var].size(); ++i) {
        int dep_var = depends_on[var][i];
        index += state[dep_var] * multiplier;
        multiplier *= g_variable_domain[dep_var];
    }
    if (to_val > from_val)
        --to_val;
    index += to_val * multiplier;
    assert(in_bounds(index, cache[var]));
    return index;
}
