#include "rcg.h"

#include "../task_proxy.h"

#include "../utils/math.h"
#include "../utils/rng.h"

#include <algorithm>
#include <unordered_set>

using namespace std;

namespace pdbs{
Pattern generate_pattern(
    int max_pdb_size,
    int goal_variable,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const TaskProxy &task_proxy,
    vector<vector<int>> &cg_neighbors) {
    int current_var = goal_variable;
    unordered_set<int> visited_vars;
    visited_vars.insert(current_var);
    int pdb_size = task_proxy.get_variables()[current_var].get_domain_size();
    while (true) {
        // Pick random cg neighbor.
        rng->shuffle(cg_neighbors[current_var]);
        int neighbor_var = -1;
        for (int candidate : cg_neighbors[current_var]) {
            if (!visited_vars.count(candidate)) {
                neighbor_var = candidate;
                break;
            }
        }

        if (neighbor_var != -1 && utils::is_product_within_limit(
            pdb_size, task_proxy.get_variables()[neighbor_var].get_domain_size(), max_pdb_size)) {
            pdb_size *= task_proxy.get_variables()[neighbor_var].get_domain_size();
            visited_vars.insert(neighbor_var);
            current_var = neighbor_var;
        } else {
            break;
        }
    }

    Pattern pattern(visited_vars.begin(), visited_vars.end());
    sort(pattern.begin(), pattern.end());
    return pattern;
}
}
