#include "rcg.h"

#include "../option_parser.h"

#include "../task_proxy.h"

#include "../utils/countdown_timer.h"
#include "../utils/logging.h"
#include "../utils/rng.h"

#include <algorithm>
#include <unordered_set>

using namespace std;

namespace pdbs{
static bool time_limit_reached(
    const utils::CountdownTimer &timer, utils::Verbosity verbosity) {
    if (timer.is_expired()) {
        if (verbosity >= utils::Verbosity::NORMAL) {
            utils::g_log << "time limit reached." << endl;
        }
        return true;
    }
    return false;
}

Pattern generate_pattern_rcg(
    int max_pdb_size,
    double max_time,
    int goal_variable,
    utils::Verbosity verbosity,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const TaskProxy &task_proxy,
    vector<vector<int>> &cg_neighbors) {
    utils::CountdownTimer timer(max_time);
    int current_var = goal_variable;
    unordered_set<int> visited_vars;
    visited_vars.insert(current_var);
    int pdb_size = task_proxy.get_variables()[current_var].get_domain_size();
    while (!time_limit_reached(timer, verbosity)) {
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

void add_rcg_options_to_parser(options::OptionParser &parser) {
    parser.add_option<int>(
        "max_pdb_size",
        "maximum number of states per pattern database (ignored for the "
        "initial collection consisting of singleton patterns for each goal "
        "variable)",
        "2000000",
        Bounds("1", "infinity"));
    parser.add_option<double>(
        "max_time",
        "maximum time in seconds for the RCG algorithm (ignored for"
        "computing initial collection)",
        "infinity",
        Bounds("0.0", "infinity"));
    parser.add_option<bool>(
        "bidirectional",
        "consider pre-eff edges of the causal graph in both directions",
        "true"
        );
}
}
