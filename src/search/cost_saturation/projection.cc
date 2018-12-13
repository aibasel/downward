#include "projection.h"

#include "types.h"

#include "../pdbs/match_tree.h"
#include "../pdbs/pattern_database.h"
#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/math.h"
#include "../utils/memory.h"

#include <cassert>
#include <unordered_map>

using namespace std;

namespace cost_saturation {
bool increment(const vector<int> &pattern_domain_sizes, vector<FactPair> &facts) {
    for (size_t i = 0; i < facts.size(); ++i) {
        ++facts[i].value;
        if (facts[i].value > pattern_domain_sizes[facts[i].var] - 1) {
            facts[i].value = 0;
        } else {
            return true;
        }
    }
    return false;
}


AbstractForwardOperator::AbstractForwardOperator(
    const vector<FactPair> &prev_pairs,
    const vector<FactPair> &pre_pairs,
    const vector<FactPair> &eff_pairs,
    const vector<size_t> &hash_multipliers,
    int concrete_operator_id)
    : concrete_operator_id(concrete_operator_id) {
    vector<int> abstract_preconditions(hash_multipliers.size(), -1);
    for (const FactPair &fact : prev_pairs) {
        int pattern_index = fact.var;
        abstract_preconditions[pattern_index] = fact.value;
    }
    for (const FactPair &fact : pre_pairs) {
        int pattern_index = fact.var;
        abstract_preconditions[pattern_index] = fact.value;
    }

    hash_effect = 0;
    assert(pre_pairs.size() == eff_pairs.size());
    for (size_t i = 0; i < pre_pairs.size(); ++i) {
        int var = pre_pairs[i].var;
        assert(var == eff_pairs[i].var);
        int old_val = pre_pairs[i].value;
        int new_val = eff_pairs[i].value;
        assert(old_val != -1);
        int effect = (new_val - old_val) * hash_multipliers[var];
        hash_effect += effect;
    }

    precondition_hash = 0;
    for (size_t pos = 0; pos < hash_multipliers.size(); ++pos) {
        int pre_val = abstract_preconditions[pos];
        if (pre_val == -1) {
            unaffected_variables.push_back(pos);
        } else {
            precondition_hash += hash_multipliers[pos] * pre_val;
        }
    }
}

int AbstractForwardOperator::get_concrete_operator_id() const {
    return concrete_operator_id;
}

Projection::Projection(
    const TaskProxy &task_proxy, const pdbs::Pattern &pattern)
    : task_proxy(task_proxy),
      pattern(pattern) {
    assert(utils::is_sorted_unique(pattern));

    active_operators = compute_active_operators();
    looping_operators = compute_looping_operators();
    assert(utils::is_sorted_unique(looping_operators));

    hash_multipliers.reserve(pattern.size());
    num_states = 1;
    for (int pattern_var_id : pattern) {
        hash_multipliers.push_back(num_states);
        VariableProxy var = task_proxy.get_variables()[pattern_var_id];
        if (utils::is_product_within_limit(num_states, var.get_domain_size(),
                                           numeric_limits<int>::max())) {
            num_states *= var.get_domain_size();
        } else {
            cerr << "Given pattern is too large! (Overflow occured): " << endl;
            cerr << pattern << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }

    VariablesProxy variables = task_proxy.get_variables();
    variable_to_pattern_index.resize(variables.size(), -1);
    for (size_t i = 0; i < pattern.size(); ++i) {
        variable_to_pattern_index[pattern[i]] = i;
    }
    pattern_domain_sizes.reserve(pattern.size());
    for (int pattern_var : pattern) {
        pattern_domain_sizes.push_back(variables[pattern_var].get_domain_size());
    }

    // Compute abstract operators.
    OperatorsProxy operators = task_proxy.get_operators();
    for (OperatorProxy op : operators) {
        build_abstract_operators(
            op, -1, variable_to_pattern_index, variables, abstract_operators);
    }
    for (OperatorProxy op : operators) {
        build_abstract_forward_operators(
            op, -1, variable_to_pattern_index, variables, abstract_forward_operators);
    }

    // Create match tree.
    match_tree = utils::make_unique_ptr<pdbs::MatchTree>(
        task_proxy, pattern, hash_multipliers);
    for (const pdbs::AbstractOperator &op : abstract_operators) {
        match_tree->insert(op);
    }

    goal_states = compute_goal_states();
}

Projection::~Projection() {
}

int Projection::get_abstract_state_id(const State &concrete_state) const {
    return hash_index(concrete_state);
}

vector<int> Projection::compute_active_operators() const {
    vector<int> active_operators;
    for (OperatorProxy op : task_proxy.get_operators()) {
        if (is_operator_relevant(op)) {
            active_operators.push_back(op.get_id());
        }
    }
    return active_operators;
}

vector<int> Projection::compute_looping_operators() const {
    vector<int> looping_operators;
    for (OperatorProxy op : task_proxy.get_operators()) {
        if (operator_induces_loop(op)) {
            looping_operators.push_back(op.get_id());
        }
    }
    return looping_operators;
}

vector<int> Projection::compute_goal_states() const {
    vector<int> goal_states;

    // Compute abstract goal var-val pairs.
    vector<FactPair> abstract_goals;
    for (FactProxy goal : task_proxy.get_goals()) {
        int var_id = goal.get_variable().get_id();
        int val = goal.get_value();
        if (variable_to_pattern_index[var_id] != -1) {
            abstract_goals.emplace_back(variable_to_pattern_index[var_id], val);
        }
    }

    for (int state_index = 0; state_index < num_states; ++state_index) {
        if (is_consistent(state_index, abstract_goals)) {
            goal_states.push_back(state_index);
        }
    }

    return goal_states;
}

void Projection::multiply_out(
    int pos, int cost, int op_id,
    vector<FactPair> &prev_pairs,
    vector<FactPair> &pre_pairs,
    vector<FactPair> &eff_pairs,
    const vector<FactPair> &effects_without_pre,
    const VariablesProxy &variables,
    vector<pdbs::AbstractOperator> &operators) const {
    if (pos == static_cast<int>(effects_without_pre.size())) {
        // All effects without precondition have been checked: insert op.
        if (!eff_pairs.empty()) {
            operators.emplace_back(
                prev_pairs, pre_pairs, eff_pairs, cost, hash_multipliers, op_id);
        }
    } else {
        // For each possible value for the current variable, build an
        // abstract operator.
        int var_id = effects_without_pre[pos].var;
        int eff = effects_without_pre[pos].value;
        VariableProxy var = variables[pattern[var_id]];
        for (int i = 0; i < var.get_domain_size(); ++i) {
            if (i != eff) {
                pre_pairs.emplace_back(var_id, i);
                eff_pairs.emplace_back(var_id, eff);
            } else {
                prev_pairs.emplace_back(var_id, i);
            }
            multiply_out(pos + 1, cost, op_id, prev_pairs, pre_pairs, eff_pairs,
                         effects_without_pre, variables, operators);
            if (i != eff) {
                pre_pairs.pop_back();
                eff_pairs.pop_back();
            } else {
                prev_pairs.pop_back();
            }
        }
    }
}

void Projection::build_abstract_operators(
    const OperatorProxy &op, int cost,
    const vector<int> &variable_to_index,
    const VariablesProxy &variables,
    vector<pdbs::AbstractOperator> &operators) const {
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
    multiply_out(0, cost, op.get_id(), prev_pairs, pre_pairs, eff_pairs,
                 effects_without_pre, variables, operators);
}

void Projection::multiply_out_forward(
    int pos, int cost, int op_id,
    vector<FactPair> &prev_pairs,
    vector<FactPair> &pre_pairs,
    vector<FactPair> &eff_pairs,
    const vector<FactPair> &effects_without_pre,
    const VariablesProxy &variables,
    vector<AbstractForwardOperator> &operators) const {
    if (pos == static_cast<int>(effects_without_pre.size())) {
        // All effects without precondition have been checked: insert op.
        if (!eff_pairs.empty()) {
            operators.emplace_back(
                prev_pairs, pre_pairs, eff_pairs, hash_multipliers, op_id);
        }
    } else {
        // For each possible value for the current variable, build an
        // abstract operator.
        int var_id = effects_without_pre[pos].var;
        int eff = effects_without_pre[pos].value;
        VariableProxy var = variables[pattern[var_id]];
        for (int i = 0; i < var.get_domain_size(); ++i) {
            if (i != eff) {
                pre_pairs.emplace_back(var_id, i);
                eff_pairs.emplace_back(var_id, eff);
            } else {
                prev_pairs.emplace_back(var_id, i);
            }
            multiply_out_forward(pos + 1, cost, op_id, prev_pairs, pre_pairs, eff_pairs,
                                 effects_without_pre, variables, operators);
            if (i != eff) {
                pre_pairs.pop_back();
                eff_pairs.pop_back();
            } else {
                prev_pairs.pop_back();
            }
        }
    }
}

void Projection::build_abstract_forward_operators(
    const OperatorProxy &op, int cost,
    const vector<int> &variable_to_index,
    const VariablesProxy &variables,
    vector<AbstractForwardOperator> &operators) const {
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
    multiply_out_forward(0, cost, op.get_id(), prev_pairs, pre_pairs, eff_pairs,
                         effects_without_pre, variables, operators);
}

vector<int> Projection::compute_distances(const vector<int> &costs) const {
    assert(all_of(costs.begin(), costs.end(), [](int c) {return c >= 0;}));
    vector<int> distances(num_states, INF);

    // Initialize queue.
    assert(pq.empty());
    for (int goal : goal_states) {
        pq.push(0, goal);
        distances[goal] = 0;
    }

    // Reuse vector to save allocations.
    vector<const pdbs::AbstractOperator *> applicable_operators;

    // Run Dijkstra loop.
    while (!pq.empty()) {
        pair<int, size_t> node = pq.pop();
        int distance = node.first;
        size_t state_index = node.second;
        assert(utils::in_bounds(state_index, distances));
        if (distance > distances[state_index]) {
            continue;
        }

        // Regress abstract state.
        applicable_operators.clear();
        match_tree->get_applicable_operators(state_index, applicable_operators);
        for (const pdbs::AbstractOperator *op : applicable_operators) {
            size_t predecessor = state_index + op->get_hash_effect();
            int op_id = op->get_concrete_operator_id();
            assert(utils::in_bounds(op_id, costs));
            int alternative_cost = (costs[op_id] == INF) ?
                INF : distances[state_index] + costs[op_id];
            assert(utils::in_bounds(predecessor, distances));
            if (alternative_cost < distances[predecessor]) {
                distances[predecessor] = alternative_cost;
                pq.push(alternative_cost, predecessor);
            }
        }
    }
    pq.clear();
    return distances;
}

bool Projection::is_consistent(
    size_t state_index,
    const vector<FactPair> &abstract_facts) const {
    for (const FactPair &abstract_goal : abstract_facts) {
        int pattern_var_id = abstract_goal.var;
        int temp = state_index / hash_multipliers[pattern_var_id];
        int val = temp % pattern_domain_sizes[pattern_var_id];
        if (val != abstract_goal.value) {
            return false;
        }
    }
    return true;
}

size_t Projection::hash_index(const State &state) const {
    size_t index = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        index += hash_multipliers[i] * state[pattern[i]].get_value();
    }
    return index;
}

bool Projection::is_operator_relevant(const OperatorProxy &op) const {
    for (EffectProxy effect : op.get_effects()) {
        int var_id = effect.get_fact().get_variable().get_id();
        if (binary_search(pattern.begin(), pattern.end(), var_id)) {
            return true;
        }
    }
    return false;
}

bool Projection::operator_induces_loop(const OperatorProxy &op) const {
    unordered_map<int, int> var_to_precondition;
    for (FactProxy precondition : op.get_preconditions()) {
        const FactPair fact = precondition.get_pair();
        var_to_precondition[fact.var] = fact.value;
    }
    for (EffectProxy effect : op.get_effects()) {
        const FactPair fact = effect.get_fact().get_pair();
        auto it = var_to_precondition.find(fact.var);
        if (it != var_to_precondition.end() &&
            it->second != fact.value &&
            binary_search(pattern.begin(), pattern.end(), fact.var)) {
            return false;
        }
    }
    return true;
}

vector<int> Projection::compute_saturated_costs(
    const vector<int> &h_values,
    int num_operators) const {
    assert(has_transition_system());
    vector<int> saturated_costs(num_operators, -INF);

    /* To prevent negative cost cycles, we ensure that all operators
       inducing self-loops have non-negative costs. */
    for (int op_id : looping_operators) {
        saturated_costs[op_id] = 0;
    }

    for_each_transition(
        [&saturated_costs, &h_values](const Transition &t) {
            assert(utils::in_bounds(t.src, h_values));
            assert(utils::in_bounds(t.target, h_values));
            int src_h = h_values[t.src];
            int target_h = h_values[t.target];
            if (src_h == INF || target_h == INF) {
                return;
            }
            int &needed_costs = saturated_costs[t.op];
            needed_costs = max(needed_costs, src_h - target_h);
        });
    return saturated_costs;
}

vector<int> Projection::compute_goal_distances(const vector<int> &costs) const {
    assert(has_transition_system());
    return compute_distances(costs);
}

int Projection::get_num_states() const {
    return num_states;
}

const vector<int> &Projection::get_active_operators() const {
    assert(has_transition_system());
    return active_operators;
}

const vector<int> &Projection::get_looping_operators() const {
    assert(has_transition_system());
    return looping_operators;
}

const vector<int> &Projection::get_goal_states() const {
    assert(has_transition_system());
    return goal_states;
}

void Projection::release_transition_system_memory() {
    utils::release_vector_memory(abstract_operators);
    utils::release_vector_memory(looping_operators);
    utils::release_vector_memory(goal_states);
    match_tree = nullptr;
}

void Projection::dump() const {
    assert(has_transition_system());
    cout << "Abstract operators: " << abstract_operators.size()
         << ", looping operators: " << looping_operators.size()
         << ", goal states: " << goal_states.size() << "/" << num_states
         << endl;
}
}
