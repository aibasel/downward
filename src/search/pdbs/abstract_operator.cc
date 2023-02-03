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

/*
  Abstract operators are built from concrete operators. The
  parameters follow the usual name convention of SAS+ operators,
  meaning prevail, preconditions and effects are all related to
  progression search.
*/
static AbstractOperator build_abstract_operator(
    const Projection &projection,
    const vector<FactPair> &prev_pairs,
    const vector<FactPair> &pre_pairs,
    const vector<FactPair> &eff_pairs,
    int concrete_op_id,
    int cost) {
    vector<FactPair> regression_preconditions(prev_pairs);
    regression_preconditions.insert(regression_preconditions.end(),
                                    eff_pairs.begin(),
                                    eff_pairs.end());
    // Sort preconditions for MatchTree construction.
    sort(regression_preconditions.begin(), regression_preconditions.end());
    for (size_t i = 1; i < regression_preconditions.size(); ++i) {
        assert(regression_preconditions[i].var !=
               regression_preconditions[i - 1].var);
    }
    int hash_effect = 0;
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
    return AbstractOperator(concrete_op_id, cost, move(regression_preconditions), hash_effect);
}

/*
  Recursive method; called by build_abstract_operators. In the case
  of a precondition with value = -1 in the concrete operator, all
  multiplied out abstract operators are computed, i.e. for all
  possible values of the variable (with precondition = -1), one
  abstract operator with a concrete value (!= -1) is computed.
*/
static void multiply_out(
    const Projection &projection,
    const VariablesProxy &variables,
    int concrete_op_id,
    int cost,
    int pos,
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
                    projection, prev_pairs, pre_pairs, eff_pairs,
                    concrete_op_id, cost));
        }
    } else {
        // For each possible value for the current variable, build an
        // abstract operator.
        int var_id = effects_without_pre[pos].var;
        int eff = effects_without_pre[pos].value;
        VariableProxy var = variables[projection.get_pattern()[var_id]];
        for (int i = 0; i < var.get_domain_size(); ++i) {
            if (i != eff) {
                pre_pairs.emplace_back(var_id, i);
                eff_pairs.emplace_back(var_id, eff);
            } else {
                prev_pairs.emplace_back(var_id, i);
            }
            multiply_out(projection, variables, concrete_op_id, cost,
                         pos + 1, prev_pairs, pre_pairs, eff_pairs,
                         effects_without_pre, operators);
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
    const OperatorProxy &op,
    int cost,
    const vector<int> &variable_to_index,
    const VariablesProxy &variables,
    vector<AbstractOperator> &operators) {
    // All variable value pairs that are a prevail condition
    vector<FactPair> prev_pairs;
    // All variable value pairs that are a precondition (value != -1)
    vector<FactPair> pre_pairs;
    // All variable value pairs that are an effect
    vector<FactPair> eff_pairs;
    // All variable value pairs that are a precondition (value = -1)
    vector<FactPair> effects_without_pre;

    size_t num_vars = variables.size();
    vector<bool> has_precond_and_effect_on_var(num_vars, false);
    vector<bool> has_precondition_on_var(num_vars, false);

    for (FactProxy pre : op.get_preconditions())
        has_precondition_on_var[pre.get_variable().get_id()] = true;

    for (EffectProxy eff : op.get_effects()) {
        int var_id = eff.get_fact().get_variable().get_id();
        int pattern_var_id = variable_to_index[var_id];
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
        int pattern_var_id = variable_to_index[var_id];
        int val = pre.get_value();
        if (pattern_var_id != -1) { // variable occurs in pattern
            if (has_precond_and_effect_on_var[var_id]) {
                pre_pairs.emplace_back(pattern_var_id, val);
            } else {
                prev_pairs.emplace_back(pattern_var_id, val);
            }
        }
    }
    multiply_out(projection, variables, op.get_id(), cost, 0,
                 prev_pairs, pre_pairs, eff_pairs, effects_without_pre,
                 operators);
}
}
