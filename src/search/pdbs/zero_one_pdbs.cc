#include "zero_one_pdbs.h"

#include "pattern_database.h"

#include "../task_proxy.h"

#include "../utils/logging.h"

#include <iostream>
#include <limits>
#include <memory>
#include <vector>

using namespace std;

namespace pdbs {
ZeroOnePDBs::ZeroOnePDBs(
    const TaskProxy &task_proxy, const PatternCollection &patterns) {
    vector<int> remaining_operator_costs;
    OperatorsProxy operators = task_proxy.get_operators();
    remaining_operator_costs.reserve(operators.size());
    for (OperatorProxy op : operators)
        remaining_operator_costs.push_back(op.get_cost());

    pattern_databases.reserve(patterns.size());
    for (const Pattern &pattern : patterns) {
        shared_ptr<PatternDatabase> pdb = make_shared<PatternDatabase>(
            task_proxy, pattern, remaining_operator_costs);

        /* Set cost of relevant operators to 0 for further iterations
           (action cost partitioning). */
        for (OperatorProxy op : operators) {
            if (pdb->is_operator_relevant(op))
                remaining_operator_costs[op.get_id()] = 0;
        }

        pattern_databases.push_back(pdb);
    }
}


int ZeroOnePDBs::get_value(const State &state) const {
    /*
      Because we use cost partitioning, we can simply add up all
      heuristic values of all patterns in the pattern collection.
    */
    state.unpack();
    int h_val = 0;
    for (const shared_ptr<PatternDatabase> &pdb : pattern_databases) {
        int pdb_value = pdb->get_value(state.get_unpacked_values());
        if (pdb_value == numeric_limits<int>::max())
            return numeric_limits<int>::max();
        h_val += pdb_value;
    }
    return h_val;
}

double ZeroOnePDBs::compute_approx_mean_finite_h() const {
    double approx_mean_finite_h = 0;
    for (const shared_ptr<PatternDatabase> &pdb : pattern_databases) {
        approx_mean_finite_h += pdb->compute_mean_finite_h();
    }
    return approx_mean_finite_h;
}

void ZeroOnePDBs::dump(utils::LogProxy &log) const {
    if (log.is_at_least_debug()) {
        for (const shared_ptr<PatternDatabase> &pdb : pattern_databases) {
            log << pdb->get_pattern() << endl;
        }
    }
}
}
