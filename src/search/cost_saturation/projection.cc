#include "projection.h"

#include "types.h"

#include "../task_proxy.h"

#include "../algorithms/priority_queues.h"
#include "../pdbs/match_tree.h"
#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/math.h"
#include "../utils/memory.h"

#include <cassert>
#include <unordered_map>

using namespace std;

namespace cost_saturation {
static vector<int> get_abstract_preconditions(
    const vector<FactPair> &prev_pairs,
    const vector<FactPair> &pre_pairs,
    const vector<size_t> &hash_multipliers) {
    vector<int> abstract_preconditions(hash_multipliers.size(), -1);
    for (const FactPair &fact : prev_pairs) {
        int pattern_index = fact.var;
        abstract_preconditions[pattern_index] = fact.value;
    }
    for (const FactPair &fact : pre_pairs) {
        int pattern_index = fact.var;
        abstract_preconditions[pattern_index] = fact.value;
    }
    return abstract_preconditions;
}

static int compute_hash_effect(
    const vector<FactPair> &preconditions,
    const vector<FactPair> &effects,
    const vector<size_t> &hash_multipliers,
    bool forward) {
    int hash_effect = 0;
    assert(preconditions.size() == effects.size());
    for (size_t i = 0; i < preconditions.size(); ++i) {
        int var = preconditions[i].var;
        assert(var == effects[i].var);
        int old_val = preconditions[i].value;
        int new_val = effects[i].value;
        assert(old_val != -1);
        if (!forward) {
            swap(old_val, new_val);
        }
        int effect = (new_val - old_val) * hash_multipliers[var];
        hash_effect += effect;
    }
    return hash_effect;
}

static vector<int> get_variables(const OperatorProxy &op) {
    unordered_set<int> vars;
    vars.reserve(op.get_preconditions().size());
    for (FactProxy precondition : op.get_preconditions()) {
        vars.insert(precondition.get_variable().get_id());
    }
    for (EffectProxy effect : op.get_effects()) {
        vars.insert(effect.get_fact().get_variable().get_id());
    }
    vector<int> variables(vars.begin(), vars.end());
    sort(variables.begin(), variables.end());
    return variables;
}

static vector<int> get_changed_variables(const OperatorProxy &op) {
    unordered_map<int, int> var_to_precondition;
    for (FactProxy precondition : op.get_preconditions()) {
        const FactPair fact = precondition.get_pair();
        var_to_precondition[fact.var] = fact.value;
    }
    vector<int> changed_variables;
    for (EffectProxy effect : op.get_effects()) {
        const FactPair fact = effect.get_fact().get_pair();
        auto it = var_to_precondition.find(fact.var);
        if (it != var_to_precondition.end() && it->second != fact.value) {
            changed_variables.push_back(fact.var);
        }
    }
    sort(changed_variables.begin(), changed_variables.end());
    return changed_variables;
}


TaskInfo::TaskInfo(const TaskProxy &task_proxy) {
    num_variables = task_proxy.get_variables().size();
    num_operators = task_proxy.get_operators().size();
    goals = task_properties::get_fact_pairs(task_proxy.get_goals());
    mentioned_variables.resize(num_operators * num_variables, false);
    pre_eff_variables.resize(num_operators * num_variables, false);
    effect_variables.resize(num_operators * num_variables, false);
    for (OperatorProxy op : task_proxy.get_operators()) {
        for (int var : get_variables(op)) {
            mentioned_variables[get_index(op.get_id(), var)] = true;
        }
        for (int changed_var : get_changed_variables(op)) {
            pre_eff_variables[get_index(op.get_id(), changed_var)] = true;
        }
        for (EffectProxy effect : op.get_effects()) {
            int var = effect.get_fact().get_variable().get_id();
            effect_variables[get_index(op.get_id(), var)] = true;
        }
    }
}

const vector<FactPair> &TaskInfo::get_goals() const {
    return goals;
}

int TaskInfo::get_num_operators() const {
    return num_operators;
}

bool TaskInfo::operator_mentions_variable(int op_id, int var) const {
    return mentioned_variables[get_index(op_id, var)];
}

bool TaskInfo::operator_induces_self_loop(const pdbs::Pattern &pattern, int op_id) const {
    // Return false iff the operator has a precondition and effect for a pattern variable.
    for (int var : pattern) {
        if (pre_eff_variables[get_index(op_id, var)]) {
            return false;
        }
    }
    return true;
}

bool TaskInfo::operator_is_active(const pdbs::Pattern &pattern, int op_id) const {
    for (int var : pattern) {
        if (effect_variables[get_index(op_id, var)]) {
            return true;
        }
    }
    return false;
}


Projection::Projection(
    const TaskProxy &task_proxy,
    const shared_ptr<TaskInfo> &task_info,
    const pdbs::Pattern &pattern)
    : task_info(task_info),
      pattern(pattern) {
    assert(utils::is_sorted_unique(pattern));

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
    vector<int> variable_to_pattern_index(variables.size(), -1);
    for (size_t i = 0; i < pattern.size(); ++i) {
        variable_to_pattern_index[pattern[i]] = i;
    }
    pattern_domain_sizes.reserve(pattern.size());
    for (int pattern_var : pattern) {
        pattern_domain_sizes.push_back(variables[pattern_var].get_domain_size());
    }

    match_tree_backward = utils::make_unique_ptr<pdbs::MatchTree>(
        task_proxy, pattern, hash_multipliers);

    // Compute abstract forward and backward operators.
    OperatorsProxy operators = task_proxy.get_operators();
    for (OperatorProxy op : operators) {
        build_abstract_operators(
            op, -1, variable_to_pattern_index, variables,
            [this](
                const vector<FactPair> &prevail,
                const vector<FactPair> &preconditions,
                const vector<FactPair> &effects,
                int,
                const vector<size_t> &hash_multipliers,
                int concrete_operator_id) {
                int abs_op_id = abstract_backward_operators.size();
                abstract_backward_operators.emplace_back(
                    concrete_operator_id,
                    compute_hash_effect(preconditions, effects, hash_multipliers, false));
                vector<FactPair> regression_preconditions = prevail;
                regression_preconditions.insert(
                    regression_preconditions.end(), effects.begin(), effects.end());
                sort(regression_preconditions.begin(), regression_preconditions.end());
                match_tree_backward->insert(abs_op_id, regression_preconditions);

                vector<int> abstract_preconditions = get_abstract_preconditions(
                    prevail, preconditions, hash_multipliers);
                int precondition_hash = 0;
                for (size_t pos = 0; pos < hash_multipliers.size(); ++pos) {
                    int pre_val = abstract_preconditions[pos];
                    if (pre_val != -1) {
                        precondition_hash += hash_multipliers[pos] * pre_val;
                    }
                }

                abstract_forward_operators.emplace_back(
                    precondition_hash,
                    compute_hash_effect(
                        preconditions, effects, hash_multipliers, true));
            });
    }
    abstract_forward_operators.shrink_to_fit();
    abstract_backward_operators.shrink_to_fit();

    goal_states = compute_goal_states(variable_to_pattern_index);
}

Projection::~Projection() {
}

bool Projection::increment_to_next_state(vector<FactPair> &facts) const {
    for (FactPair &fact : facts) {
        ++fact.value;
        if (fact.value > pattern_domain_sizes[fact.var] - 1) {
            fact.value = 0;
        } else {
            return true;
        }
    }
    return false;
}

int Projection::get_abstract_state_id(const State &concrete_state) const {
    size_t index = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        index += hash_multipliers[i] * concrete_state[pattern[i]].get_value();
    }
    return index;
}

vector<int> Projection::compute_goal_states(
    const vector<int> &variable_to_pattern_index) const {
    vector<FactPair> abstract_goals;
    for (FactPair goal : task_info->get_goals()) {
        if (variable_to_pattern_index[goal.var] != -1) {
            abstract_goals.emplace_back(
                variable_to_pattern_index[goal.var], goal.value);
        }
    }

    vector<int> goal_states;
    for (int state_index = 0; state_index < num_states; ++state_index) {
        if (is_consistent(state_index, abstract_goals)) {
            goal_states.push_back(state_index);
        }
    }
    return goal_states;
}

void Projection::multiply_out(int pos, int cost, int op_id,
                              vector<FactPair> &prev_pairs,
                              vector<FactPair> &pre_pairs,
                              vector<FactPair> &eff_pairs,
                              const vector<FactPair> &effects_without_pre,
                              const VariablesProxy &variables,
                              const OperatorCallback &callback) const {
    if (pos == static_cast<int>(effects_without_pre.size())) {
        // All effects without precondition have been checked.
        if (!eff_pairs.empty()) {
            callback(prev_pairs, pre_pairs, eff_pairs, cost, hash_multipliers, op_id);
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
                         effects_without_pre, variables, callback);
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
    const OperatorProxy &op,
    int cost,
    const vector<int> &variable_to_pattern_index,
    const VariablesProxy &variables,
    const OperatorCallback &callback) const {
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
        int pattern_var_id = variable_to_pattern_index[var_id];
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
        int pattern_var_id = variable_to_pattern_index[var_id];
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
                 effects_without_pre, variables, callback);
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

vector<int> Projection::compute_saturated_costs(
    const vector<int> &h_values) const {
    assert(has_transition_system());
    int num_operators = task_info->get_num_operators();
    vector<int> saturated_costs(num_operators, -INF);

    /* To prevent negative cost cycles, we ensure that all operators
       inducing self-loops have non-negative costs. */
    for (int op_id = 0; op_id < num_operators; ++op_id) {
        if (operator_induces_self_loop(op_id)) {
            saturated_costs[op_id] = 0;
        }
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
    assert(all_of(costs.begin(), costs.end(), [](int c) {return c >= 0;}));
    vector<int> distances(num_states, INF);

    // Initialize queue.
    priority_queues::AdaptiveQueue<size_t> pq;
    for (int goal : goal_states) {
        pq.push(0, goal);
        distances[goal] = 0;
    }

    // Reuse vector to save allocations.
    vector<int> applicable_operator_ids;

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
        applicable_operator_ids.clear();
        match_tree_backward->get_applicable_operator_ids(
            state_index, applicable_operator_ids);
        for (int abs_op_id : applicable_operator_ids) {
            const AbstractBackwardOperator &op = abstract_backward_operators[abs_op_id];
            size_t predecessor = state_index + op.hash_effect;
            int conc_op_id = op.concrete_operator_id;
            assert(utils::in_bounds(conc_op_id, costs));
            int alternative_cost = (costs[conc_op_id] == INF) ?
                INF : distances[state_index] + costs[conc_op_id];
            assert(utils::in_bounds(predecessor, distances));
            if (alternative_cost < distances[predecessor]) {
                distances[predecessor] = alternative_cost;
                pq.push(alternative_cost, predecessor);
            }
        }
    }
    return distances;
}

int Projection::get_num_states() const {
    return num_states;
}

bool Projection::operator_is_active(int op_id) const {
    return task_info->operator_is_active(pattern, op_id);
}

bool Projection::operator_induces_self_loop(int op_id) const {
    return task_info->operator_induces_self_loop(pattern, op_id);
}

const vector<int> &Projection::get_goal_states() const {
    assert(has_transition_system());
    return goal_states;
}

void Projection::release_transition_system_memory() {
    assert(has_transition_system());
    utils::release_vector_memory(abstract_forward_operators);
    utils::release_vector_memory(abstract_backward_operators);
    utils::release_vector_memory(goal_states);
    match_tree_backward = nullptr;
}

void Projection::dump() const {
    assert(has_transition_system());
    cout << "Abstract operators: " << abstract_backward_operators.size()
         << ", goal states: " << goal_states.size() << "/" << num_states
         << endl;
}
}
