#include "zero_one_pdbs.h"

#include "pattern_database.h"

#include "../task_proxy.h"

#include <limits>
#include <vector>

using namespace std;


ZeroOnePDBs::ZeroOnePDBs(TaskProxy task_proxy, const Patterns &patterns) {
    vector<int> operator_costs;
    OperatorsProxy operators = task_proxy.get_operators();
    operator_costs.reserve(operators.size());
    for (OperatorProxy op : operators)
        operator_costs.push_back(op.get_cost());

    //Timer timer;
    pattern_databases.reserve(patterns.size());
    for (const vector<int> &pattern : patterns) {
        unique_ptr<PatternDatabase> pdb = make_unique_ptr<PatternDatabase>(
            task_proxy, pattern, false, operator_costs);

        /* Set cost of relevant operators to 0 for further iterations
           (action cost partitioning). */
        for (OperatorProxy op : operators) {
            if (pdb->is_operator_relevant(op))
                operator_costs[op.get_id()] = 0;
        }

        pattern_databases.push_back(move(pdb));
    }
    //cout << "All or nothing PDB collection construction time: " <<
    //timer << endl;
}


int ZeroOnePDBs::get_value(const State &state) {
    /*
      Because we use cost partitioning, we can simply add up all
      heuristic values of all patterns in the pattern collection.
    */
    int h_val = 0;
    for (const auto &pdb : pattern_databases) {
        int pdb_value = pdb->get_value(state);
        if (pdb_value == numeric_limits<int>::max())
            return numeric_limits<int>::max();
        h_val += pdb_value;
    }
    return h_val;
}

double ZeroOnePDBs::compute_approx_mean_finite_h() const {
    double approx_mean_finite_h = 0;
    for (const auto &pdb : pattern_databases) {
        approx_mean_finite_h += pdb->compute_mean_finite_h();
    }
    return approx_mean_finite_h;
}

void ZeroOnePDBs::dump() const {
    for (const auto &pdb : pattern_databases) {
        cout << pdb->get_pattern() << endl;
    }
}
