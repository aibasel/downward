#include "random_pattern.h"

#include "../option_parser.h"

#include "../task_proxy.h"

#include "../utils/countdown_timer.h"
#include "../utils/logging.h"
#include "../utils/rng.h"

#include <algorithm>
#include <unordered_set>

using namespace std;

namespace pdbs {
static bool time_limit_reached(
    const utils::CountdownTimer &timer, utils::Verbosity verbosity) {
    if (timer.is_expired()) {
        if (verbosity >= utils::Verbosity::NORMAL) {
            utils::g_log << "Random pattern generation time limit reached" << endl;
        }
        return true;
    }
    return false;
}

Pattern generate_random_pattern(
    int max_pdb_size,
    double max_time,
    utils::Verbosity verbosity,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const TaskProxy &task_proxy,
    int goal_variable,
    vector<vector<int>> &cg_neighbors) {
    utils::CountdownTimer timer(max_time);
    int current_var = goal_variable;
    unordered_set<int> visited_vars;
    visited_vars.insert(current_var);
    VariablesProxy variables = task_proxy.get_variables();
    int pdb_size = variables[current_var].get_domain_size();
    while (!time_limit_reached(timer, verbosity)) {
        rng->shuffle(cg_neighbors[current_var]);

        /*
          Search for a neighbor which was not selected yet and which fits the
          size limit.
        */
        bool found_neighbor = false;
        for (int neighbor : cg_neighbors[current_var]) {
            int neighbor_dom_size = variables[neighbor].get_domain_size();
            if (!visited_vars.count(neighbor) && utils::is_product_within_limit(
                    pdb_size, neighbor_dom_size, max_pdb_size)) {
                pdb_size *= neighbor_dom_size;
                visited_vars.insert(neighbor);
                current_var = neighbor;
                found_neighbor = true;
                break;
            }
        }

        if (!found_neighbor) {
            break;
        }
    }

    Pattern pattern(visited_vars.begin(), visited_vars.end());
    sort(pattern.begin(), pattern.end());
    return pattern;
}

void add_random_pattern_implementation_notes_to_parser(
    options::OptionParser &parser) {
    parser.document_note(
        "Short description of the random pattern algorithm",
        "The random pattern algorithm computes a pattern for a given planning "
        "task and a single goal of the task as follows. Starting with the given "
        "goal variable, the algorithm executes a random walk on the causal "
        "graph. In each iteration, it selects a random causal graph neighbor of "
        "the current variable. It terminates if no neighbor fits the pattern due "
        "to the size limit or if the time limit is reached.",
        true);
    parser.document_note(
        "Implementation notes about the random pattern algorithm",
        "In the original implementation used in the paper, the algorithm "
        "selected a random neighbor and then checked if selecting it would "
        "violate the PDB size limit. If so, the algorithm would not select "
        "it and terminate. In the current implementation, the algorithm instead "
        "loops over all neighbors of the current variable in random order and "
        "selects the first one not violating the PDB size limit. If no such "
        "neighbor exists, the algorithm terminates.",
        true);
}

void add_random_pattern_bidirectional_option_to_parser(options::OptionParser &parser) {
    parser.add_option<bool>(
        "bidirectional",
        "this option decides if the causal graph is considered to be "
        "directed or undirected selecting predecessors of already selected "
        "variables. If true (default), it is considered to be undirected "
        "(precondition-effect edges are bidirectional). If false, it is "
        "considered to be directed (a variable is a neighbor only if it is a "
        "predecessor.",
        "true");
}
}
