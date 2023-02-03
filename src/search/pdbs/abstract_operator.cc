#include "abstract_operator.h"

#include "pattern_database.h"

#include "../utils/logging.h"

using namespace std;

namespace pdbs {
AbstractOperator::AbstractOperator(const vector<FactPair> &prev_pairs,
                                   const vector<FactPair> &pre_pairs,
                                   const vector<FactPair> &eff_pairs,
                                   int cost,
                                   const Projection &projection,
                                   int concrete_op_id)
    : concrete_op_id(concrete_op_id),
      cost(cost),
      regression_preconditions(prev_pairs) {
    regression_preconditions.insert(regression_preconditions.end(),
                                    eff_pairs.begin(),
                                    eff_pairs.end());
    // Sort preconditions for MatchTree construction.
    sort(regression_preconditions.begin(), regression_preconditions.end());
    for (size_t i = 1; i < regression_preconditions.size(); ++i) {
        assert(regression_preconditions[i].var !=
               regression_preconditions[i - 1].var);
    }
    hash_effect = 0;
    assert(pre_pairs.size() == eff_pairs.size());
    for (size_t i = 0; i < pre_pairs.size(); ++i) {
        int var = pre_pairs[i].var;
        assert(var == eff_pairs[i].var);
        int old_val = eff_pairs[i].value;
        int new_val = pre_pairs[i].value;
        assert(new_val != -1);
        int effect = (new_val - old_val) * projection.get_multiplier(var);
        hash_effect += effect;
    }
}

void AbstractOperator::dump(const Pattern &pattern,
                            const VariablesProxy &variables,
                            utils::LogProxy &log) const {
    if (log.is_at_least_debug()) {
        log << "AbstractOperator:" << endl;
        log << "Regression preconditions:" << endl;
        for (size_t i = 0; i < regression_preconditions.size(); ++i) {
            int var_id = regression_preconditions[i].var;
            int val = regression_preconditions[i].value;
            log << "Variable: " << var_id << " (True name: "
                << variables[pattern[var_id]].get_name()
                << ", Index: " << i << ") Value: " << val << endl;
        }
        log << "Hash effect:" << hash_effect << endl;
    }
}
}
