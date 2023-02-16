#include "abstract_operator.h"

#include "pattern_database.h"

#include "../utils/logging.h"

using namespace std;

namespace pdbs {
AbstractOperator::AbstractOperator(
    int concrete_op_id,
    int cost,
    vector<FactPair> &&regression_preconditions,
    int hash_effect)
    : concrete_op_id(concrete_op_id),
      cost(cost),
      regression_preconditions(move(regression_preconditions)),
      hash_effect(hash_effect) {
}

void AbstractOperator::dump(const Pattern &pattern,
                            const VariablesProxy &variables,
                            utils::LogProxy &log) const {
    if (log.is_at_least_debug()) {
        log << "AbstractOperator:" << endl;
        log << "Preconditions:" << endl;
        for (size_t i = 0; i < regression_preconditions.size(); ++i) {
            int var_id = regression_preconditions[i].var;
            int val = regression_preconditions[i].value;
            log << "Pattern variable: " << var_id
                << " (Concrete variable: " << pattern[var_id]
                << "  True name: " << variables[pattern[var_id]].get_name()
                << ") Value: " << val << endl;
        }
        log << "Hash effect: " << hash_effect << endl;
    }
}
}
