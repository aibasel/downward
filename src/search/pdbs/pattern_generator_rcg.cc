#include "pattern_generator_rcg.h"

#include "../utils/math.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <algorithm>

using namespace std;

namespace pdbs{

PatternGeneratorRCG::PatternGeneratorRCG(
        shared_ptr<utils::RandomNumberGenerator> arg_rng,
        vector<vector<int>> &arg_neighborhoods,
        int arg_max_pdb_size,
        int start_goal_variable)
        : rng(arg_rng),
          neighborhoods(arg_neighborhoods),
          max_pdb_size(arg_max_pdb_size),
          start_goal_variable(start_goal_variable) {
}

bool PatternGeneratorRCG::can_merge(
        const TaskProxy &tp, const Pattern &P, int new_var) {
    unsigned int num_states = static_cast<unsigned int>(
            tp.get_variables()[new_var].get_domain_size());

    for (auto vid : P) {
        VariableProxy v = tp.get_variables()[vid];

        if (utils::is_product_within_limit(num_states, v.get_domain_size(),
                                           max_pdb_size)) {
            num_states *= v.get_domain_size();
        } else {
            return false;
        }
    }

    return true;
}

Pattern PatternGeneratorRCG::generate(
        const std::shared_ptr<AbstractTask> &task) {
    TaskProxy tp(*task);

    unordered_set<int> visited;
    Pattern P{start_goal_variable};
    int head = start_goal_variable;

    visited.insert(head);
    while (true) {
        // pick random unvisited successor of head
        rng->shuffle(neighborhoods[head]);
        int next_head = -1;
        for (int candidate : neighborhoods[head]) {
            if (!visited.count(candidate)) {
                next_head = candidate;
                break;
            }
        }

        // abort if no valid successor node could be found
        // or if pattern will become too large
        if (next_head == -1 || !can_merge(tp, P, next_head)) {
            break;
        }

        P.push_back(next_head);
        visited.insert(next_head);
        head = next_head;
    }

    std::sort(P.begin(),P.end());
    return P;
}

}