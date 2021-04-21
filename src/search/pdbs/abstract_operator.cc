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

size_t AbstractOperator::apply_to_state(size_t state_index) const {
    int succ = static_cast<int>(state_index);
    succ += hash_effect;
    assert(succ >= 0);
    return static_cast<size_t>(succ);
}

void AbstractOperator::dump(const Pattern &pattern,
                            const VariablesProxy &variables) const {
    utils::g_log << "AbstractOperator:" << endl;
    utils::g_log << "Preconditions:" << endl;
    for (size_t i = 0; i < preconditions.size(); ++i) {
        int var_id = preconditions[i].var;
        int val = preconditions[i].value;
        utils::g_log << "Pattern variable: " << var_id
                     << " (Concrete variable: " << pattern[var_id]
                     << "  True name: " << variables[pattern[var_id]].get_name()
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
    const vector<FactPair> &prev_pairs,
    const vector<FactPair> &pre_pairs,
    const vector<FactPair> &eff_pairs,
    int cost,
    const PerfectHashFunction &hash_function,
    bool progression) {
    vector<FactPair> preconditions(prev_pairs);
    if (progression) {
        preconditions.insert(preconditions.end(),
                             pre_pairs.begin(),
                             pre_pairs.end());
    } else {
        preconditions.insert(preconditions.end(),
                             eff_pairs.begin(),
                             eff_pairs.end());
    }
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
        int old_val;
        int new_val;
        if (progression) {
            old_val = pre_pairs[i].value;
            new_val = eff_pairs[i].value;
        } else {
            old_val = eff_pairs[i].value;
            new_val = pre_pairs[i].value;
        }
        assert(new_val != -1);
        int effect = (new_val - old_val) * hash_function.get_multiplier(var);
        hash_effect += effect;
    }
    return AbstractOperator(cost, move(preconditions), hash_effect);
}

AbstractOperators::AbstractOperators(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    OperatorType operator_type,
    const vector<int> &operator_costs,
    bool compute_op_id_mapping)
    : projection(projection),
      hash_function(hash_function),
      operator_type(operator_type) {
    compute_abstract_operators(operator_costs, compute_op_id_mapping);
}

void AbstractOperators::multiply_out(
    int pos,
    int cost,
    vector<FactPair> &prev_pairs,
    vector<FactPair> &pre_pairs,
    vector<FactPair> &eff_pairs,
    const vector<FactPair> &effects_without_pre) {
    if (pos == static_cast<int>(effects_without_pre.size())) {
        // All effects without precondition have been checked: insert op.
        if (!eff_pairs.empty()) {
            if (operator_type == OperatorType::Progression ||
                operator_type == OperatorType::RegressionAndProgression) {
                progression_operators.emplace_back(
                    build_abstract_operator(
                        prev_pairs, pre_pairs, eff_pairs, cost,
                        hash_function, true));
            }
            if (operator_type == OperatorType::Regression ||
                operator_type == OperatorType::RegressionAndProgression) {
                regression_operators.emplace_back(
                    build_abstract_operator(
                        prev_pairs, pre_pairs, eff_pairs, cost,
                        hash_function, false));
            }
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
            multiply_out(pos + 1, cost, prev_pairs, pre_pairs, eff_pairs,
                         effects_without_pre);
            if (i != eff) {
                pre_pairs.pop_back();
                eff_pairs.pop_back();
            } else {
                prev_pairs.pop_back();
            }
        }
    }
}

void AbstractOperators::build_abstract_operators(
    const OperatorProxy &op, int cost) {
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
    multiply_out(0, cost, prev_pairs, pre_pairs, eff_pairs, effects_without_pre);
}

void AbstractOperators::compute_abstract_operators(
    const vector<int> &operator_costs, bool compute_op_id_mapping) {
    // compute all abstract operators
    for (OperatorProxy op : projection.get_task_proxy().get_operators()) {
        int op_cost;
        if (operator_costs.empty()) {
            op_cost = op.get_cost();
        } else {
            op_cost = operator_costs[op.get_id()];
        }
        size_t previous_num_abstract_operators;
        if (compute_op_id_mapping) {
            if (operator_type == OperatorType::Progression ||
                operator_type == OperatorType::RegressionAndProgression) {
                previous_num_abstract_operators = progression_operators.size();
            } else {
                assert(operator_type == OperatorType::Regression);
                previous_num_abstract_operators = regression_operators.size();
            }
        }
        build_abstract_operators(op, op_cost);
        if (compute_op_id_mapping) {
            size_t new_num_abstract_operators;
            if (operator_type == OperatorType::Progression ||
                operator_type == OperatorType::RegressionAndProgression) {
                new_num_abstract_operators = progression_operators.size();
            } else {
                assert(operator_type == OperatorType::Regression);
                new_num_abstract_operators = regression_operators.size();
            }
            size_t num_induced_abstract_ops = new_num_abstract_operators - previous_num_abstract_operators;
            for (size_t i = 0; i < num_induced_abstract_ops; ++i) {
                abstract_to_concrete_op_ids.emplace_back(op.get_id());
            }
        }
    }
}
}
