#include "abstract_operator.h"

#include "pattern_database.h"
#include "pattern_database_factory.h"

#include "../utils/logging.h"

using namespace std;

namespace pdbs {
AbstractOperator::AbstractOperator(
    int cost,
    vector<FactPair> &&preconditions,
    int hash_effect)
    : cost(cost),
      preconditions(move(preconditions)),
      hash_effect(hash_effect) {
}

AbstractOperator::~AbstractOperator() {
}

void AbstractOperator::dump(const Projection &projection) const {
    utils::g_log << "AbstractOperator:" << endl;
    utils::g_log << "Preconditions:" << endl;
    for (size_t i = 0; i < preconditions.size(); ++i) {
        int var_id = preconditions[i].var;
        int val = preconditions[i].value;
        utils::g_log << "Pattern variable: " << var_id
                     << " (Concrete variable: " << projection.get_pattern()[var_id]
                     << "  True name: " << projection.get_task_proxy().get_variables()[projection.get_pattern()[var_id]].get_name()
                     << ") Value: " << val << endl;
    }
    utils::g_log << "Hash effect: " << hash_effect << endl;
}

/*
  Abstract operators are built from concrete operators. The
  parameters follow the usual name convention of SAS+ operators,
  meaning prevail, preconditions and effects are all related to
  progression search.
*/
AbstractOperator build_abstract_operator(
    const PerfectHashFunction &hash_function,
    const vector<FactPair> &prev_pairs,
    const vector<FactPair> &pre_pairs,
    const vector<FactPair> &eff_pairs,
    int cost) {
    vector<FactPair> preconditions(prev_pairs);
    preconditions.insert(preconditions.end(),
                         eff_pairs.begin(),
                         eff_pairs.end());
    // Sort preconditions for MatchTree construction.
    sort(preconditions.begin(), preconditions.end());
    for (size_t i = 1; i < preconditions.size(); ++i) {
        assert(preconditions[i].var !=
               preconditions[i - 1].var);
    }
    int hash_effect = 0;
    assert(pre_pairs.size() == eff_pairs.size());
    for (size_t i = 0; i < pre_pairs.size(); ++i) {
        int var = pre_pairs[i].var;
        assert(var == eff_pairs[i].var);
        int old_val = eff_pairs[i].value;
        int new_val = pre_pairs[i].value;
        assert(new_val != -1);
        int effect = (new_val - old_val) * hash_function.get_multiplier(var);
        hash_effect += effect;
    }
    return AbstractOperator(cost, move(preconditions), hash_effect);
}

/*
  Recursive method; called by build_abstract_operators. In the case
  of a precondition with value = -1 in the concrete operator, all
  multiplied out abstract operators are computed, i.e. for all
  possible values of the variable (with precondition = -1), one
  abstract operator with a concrete value (!= -1) is computed.
*/
void multiply_out(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    int pos,
    int cost,
    vector<FactPair> &prev_pairs,
    vector<FactPair> &pre_pairs,
    vector<FactPair> &eff_pairs,
    const vector<FactPair> &effects_without_pre,
    vector<AbstractOperator> &operators) {
    if (pos == static_cast<int>(effects_without_pre.size())) {
        // All effects without precondition have been checked: insert op.
        if (!eff_pairs.empty()) {
            operators.emplace_back(
                build_abstract_operator(
                    hash_function, prev_pairs, pre_pairs, eff_pairs, cost));
        }
    } else {
        // For each possible value for the current variable, build an
        // abstract operator.
        int var_id = effects_without_pre[pos].var;
        int eff = effects_without_pre[pos].value;
        VariablesProxy variables = projection.get_task_proxy().get_variables();
        VariableProxy var = variables[projection.get_pattern()[var_id]];
        for (int i = 0; i < var.get_domain_size(); ++i) {
            if (i != eff) {
                pre_pairs.emplace_back(var_id, i);
                eff_pairs.emplace_back(var_id, eff);
            } else {
                prev_pairs.emplace_back(var_id, i);
            }
            multiply_out(projection, hash_function, pos + 1, cost, prev_pairs,
                         pre_pairs, eff_pairs, effects_without_pre, operators);
            if (i != eff) {
                pre_pairs.pop_back();
                eff_pairs.pop_back();
            } else {
                prev_pairs.pop_back();
            }
        }
    }
}

/*
  Computes all abstract operators for a given concrete operator.
  Initializes data structures for initial call to recursive method
  multiply_out.
*/
void build_abstract_operators(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const OperatorProxy &op,
    int cost,
    vector<AbstractOperator> &operators) {
    // All variable value pairs that are a prevail condition
    vector<FactPair> prev_pairs;
    // All variable value pairs that are a precondition (value != -1)
    vector<FactPair> pre_pairs;
    // All variable value pairs that are an effect
    vector<FactPair> eff_pairs;
    // All variable value pairs that are a precondition (value = -1)
    vector<FactPair> effects_without_pre;

    size_t num_vars = projection.get_task_proxy().get_variables().size();
    vector<bool> has_precond_and_effect_on_var(num_vars, false);
    vector<bool> has_precondition_on_var(num_vars, false);

    for (FactProxy pre : op.get_preconditions())
        has_precondition_on_var[pre.get_variable().get_id()] = true;

    for (EffectProxy eff : op.get_effects()) {
        int var_id = eff.get_fact().get_variable().get_id();
        int pattern_var_id = projection.get_pattern_index(var_id);
        int val = eff.get_fact().get_value();
        if (pattern_var_id != -1) {
            if (has_precondition_on_var[var_id]) {
                has_precond_and_effect_on_var[var_id] = true;
                eff_pairs.emplace_back(pattern_var_id, val);
            } else {
                effects_without_pre.emplace_back(pattern_var_id, val);
            }
        }
    }
    for (FactProxy pre : op.get_preconditions()) {
        int var_id = pre.get_variable().get_id();
        int pattern_var_id = projection.get_pattern_index(var_id);
        int val = pre.get_value();
        if (pattern_var_id != -1) { // variable occurs in pattern
            if (has_precond_and_effect_on_var[var_id]) {
                pre_pairs.emplace_back(pattern_var_id, val);
            } else {
                prev_pairs.emplace_back(pattern_var_id, val);
            }
        }
    }
    multiply_out(projection, hash_function, 0, cost, prev_pairs,
                 pre_pairs, eff_pairs, effects_without_pre, operators);
}

vector<AbstractOperator> compute_abstract_operators(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const vector<int> &operator_costs) {
    vector<AbstractOperator> operators;
    for (OperatorProxy op : projection.get_task_proxy().get_operators()) {
        int op_cost;
        if (operator_costs.empty()) {
            op_cost = op.get_cost();
        } else {
            op_cost = operator_costs[op.get_id()];
        }
        build_abstract_operators(projection, hash_function, op, op_cost, operators);
    }
    return operators;
}
}
