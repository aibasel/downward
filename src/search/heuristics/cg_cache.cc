#include "cg_cache.h"

#include "../task_proxy.h"

#include "../task_utils/causal_graph.h"
#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/math.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

using namespace std;

namespace cg_heuristic {
const int CGCache::NOT_COMPUTED;

CGCache::CGCache(const TaskProxy &task_proxy, int max_cache_size)
    : task_proxy(task_proxy) {
    utils::g_log << "Initializing heuristic cache... " << flush;

    int var_count = task_proxy.get_variables().size();
    const causal_graph::CausalGraph &cg = task_proxy.get_causal_graph();

    // Compute inverted causal graph.
    depends_on.resize(var_count);
    for (int var = 0; var < var_count; ++var) {
        for (auto succ_var : cg.get_pre_to_eff(var)) {
            // Ignore arcs that are not part of the reduced CG:
            // These are ignored by the CG heuristic.
            if (succ_var > var)
                depends_on[succ_var].push_back(var);
        }
    }

    // Compute transitive closure of inverted reduced causal graph.
    // This is made easier because it is acyclic and the variables
    // are in topological order.
    for (int var = 0; var < var_count; ++var) {
        size_t num_affectors = depends_on[var].size();
        for (size_t i = 0; i < num_affectors; ++i) {
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
            var, depends_on[var], max_cache_size);
        if (required_cache_size != -1) {
            cache[var].resize(required_cache_size, NOT_COMPUTED);
            helpful_transition_cache[var].resize(required_cache_size, nullptr);
        }
    }

    utils::g_log << "done!" << endl;
}

CGCache::~CGCache() {
}

int CGCache::compute_required_cache_size(
    int var_id, const vector<int> &depends_on, int max_cache_size) const {
    /*
      Compute the size of the cache required for variable with ID "var_id",
      which depends on the variables in "depends_on". Requires that the caches
      for all variables in "depends_on" have already been allocated. Returns -1
      if the variable cannot be cached because the required cache size would be
      too large.
    */

    VariablesProxy variables = task_proxy.get_variables();
    int var_domain = variables[var_id].get_domain_size();
    if (!utils::is_product_within_limit(var_domain, var_domain - 1,
                                        max_cache_size))
        return -1;

    int required_size = var_domain * (var_domain - 1);

    for (int depend_var_id : depends_on) {
        int depend_var_domain = variables[depend_var_id].get_domain_size();

        /*
          If var depends on a variable var_i that is not cached, then
          it cannot be cached. This is possible even if var would have
          an acceptable cache size because the domain of var_i
          contributes quadratically to its own cache size but only
          linearly to the cache size of var.
        */
        if (cache[depend_var_id].empty())
            return -1;

        if (!utils::is_product_within_limit(required_size, depend_var_domain,
                                            max_cache_size))
            return -1;

        required_size *= depend_var_domain;
    }

    return required_size;
}

int CGCache::get_index(int var, const State &state,
                       int from_val, int to_val) const {
    assert(is_cached(var));
    assert(from_val != to_val);
    int index = from_val;
    int multiplier = task_proxy.get_variables()[var].get_domain_size();
    for (int dep_var : depends_on[var]) {
        index += state[dep_var].get_value() * multiplier;
        multiplier *= task_proxy.get_variables()[dep_var].get_domain_size();
    }
    if (to_val > from_val)
        --to_val;
    index += to_val * multiplier;
    assert(utils::in_bounds(index, cache[var]));
    return index;
}
}
