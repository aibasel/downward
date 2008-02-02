#include "cache.h"
#include "causal_graph.h"
#include "globals.h"
#include "state.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>
using namespace std;

const int Cache::NOT_COMPUTED;

Cache::Cache() {
    cout << "Initializing heuristic cache... " << flush;

    int var_count = g_variable_domain.size();
    CausalGraph *cg = g_causal_graph;

    // Compute inverted causal graph.
    depends_on.resize(var_count);
    for(int var = 0; var < var_count; var++) {
        // This is to be on the safe side of overflows for the multiplications below.
        assert(g_variable_domain[var] <= 1000);
        const vector<int> &succ = cg->get_successors(var);
        for(int i = 0; i < succ.size(); i++) {
	    // Ignore arcs that are not part of the reduced CG:
	    // These are ignored by the CG heuristic.
	    if(succ[i] > var)
	        depends_on[succ[i]].push_back(var);
	}
    }

    // Compute transitive close of inverted causal graph.
    // This is made easier because it is acyclic and the variables
    // are in topological order.
    for(int var = 0; var < var_count; var++) {
        int direct_depend_count = depends_on[var].size();
        for(int i = 0; i < direct_depend_count; i++) {
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

    /*
      cout << "Dumping transitive closure of inverted causal graph..." << endl;
      for(int var = 0; var < var_count; var++) {
      cout << var << ":";
      for(int i = 0; i < depends_on[var].size(); i++)
      cout << " " << depends_on[var][i];
      cout << endl;
      }
    */

    const int MAX_CACHE_SIZE = 1000000;

    cache.resize(var_count);
    helpful_transition_cache.resize(var_count);

    for(int var = 0; var < var_count; var++) {
        int required_cache_size = g_variable_domain[var] * (g_variable_domain[var] - 1);
        if(required_cache_size <= MAX_CACHE_SIZE) {
            for(int i = 0; i < depends_on[var].size(); i++) {
                required_cache_size *= g_variable_domain[depends_on[var][i]];
                if(required_cache_size > MAX_CACHE_SIZE)
                    break;
            }
        }
        if(required_cache_size <= MAX_CACHE_SIZE) {
            //  cout << "Cache for var " << var << ": "
            //       << required_cache_size << " entries" << endl;
            cache[var].resize(required_cache_size, NOT_COMPUTED);
            helpful_transition_cache[var].resize(required_cache_size, 0);
        }
    }

    cout << "done!" << endl;
}

int Cache::get_index(int var, const State &state, int from_val, int to_val) const {
    assert(is_cached(var));
    assert(from_val != to_val);
    int index = from_val;
    int multiplier = g_variable_domain[var];
    for(int i = 0; i < depends_on[var].size(); i++) {
        int dep_var = depends_on[var][i];
        index += state[dep_var] * multiplier;
        multiplier *= g_variable_domain[dep_var];
    }
    if(to_val > from_val)
        to_val--;
    index += to_val * multiplier;
    assert(index >= 0 && index < cache[var].size());
    return index;
}
