#include "cg_cache.h"

#include "causal_graph.h"
#include "global_state.h"
#include "globals.h"
#include "task_proxy.h"
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
    /* TODO when switching merge and shrink to new task interface, use a
       task passed as a parameter instead of g_root_task(). */
    TaskProxy task_proxy(*g_root_task());
    const CausalGraph &cg = task_proxy.get_causal_graph();

    // Compute inverted causal graph.
    depends_on.resize(var_count);
    for (int var = 0; var < var_count; ++var) {
        const vector<int> &succ = cg.get_pre_to_eff(var);
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

    cache.resize(var_count);
    helpful_transition_cache.resize(var_count);

    for (int var = 0; var < var_count; ++var) {
        int required_cache_size = compute_required_cache_size(
            var, depends_on[var]);
        if (required_cache_size != -1) {
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

int CGCache::compute_required_cache_size(
    int var, const vector<int> &depends_on) const {
    /*
      Compute the size of the cache required for variable "var", which
      depends on the variables in "depends_on". Requires that the
      caches for all variables in "depends_on" have already been
      allocated. Returns -1 if the variable cannot be cached because
      the required cache size would be too large.
    */

    const int MAX_CACHE_SIZE = 1000000;

    int var_domain = g_variable_domain[var];
    if (!is_product_within_limit(var_domain, var_domain - 1, MAX_CACHE_SIZE))
        return -1;

    int required_size = var_domain * (var_domain - 1);

    for (size_t i = 0; i < depends_on.size(); ++i) {
        int depend_var = depends_on[i];
        int depend_var_domain = g_variable_domain[depend_var];

        /*
          If var depends on a variable var_i that is not cached, then
          it cannot be cached. This is possible even if var would have
          an acceptable cache size because the domain of var_i
          contributes quadratically to its own cache size but only
          linearly to the cache size of var.
        */
        if (cache[depend_var].empty())
            return -1;

        if (!is_product_within_limit(required_size, depend_var_domain,
                                     MAX_CACHE_SIZE))
            return -1;

        required_size *= depend_var_domain;
    }

    return required_size;
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
