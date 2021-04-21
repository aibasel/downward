#include "pattern_database_factory.h"

#include "abstract_operator.h"
#include "match_tree.h"
#include "pattern_database.h"

#include "../algorithms/priority_queues.h"
#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/math.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace std;

namespace pdbs {
Projection::Projection(
    const TaskProxy &task_proxy, const Pattern &pattern)
    : task_proxy(task_proxy), pattern(pattern) {
}

void Projection::compute_variable_to_index() const {
    variable_to_index.resize(task_proxy.get_variables().size(), -1);
    for (size_t i = 0; i < pattern.size(); ++i) {
        variable_to_index[pattern[i]] = i;
    }
}

void Projection::compute_abstract_goals() const {
    for (FactProxy goal : task_proxy.get_goals()) {
        int var_id = goal.get_variable().get_id();
        int val = goal.get_value();
        if (variable_to_index[var_id] != -1) {
            abstract_goals.emplace_back(variable_to_index[var_id], val);
        }
    }
}

PerfectHashFunction compute_hash_function(
    const TaskProxy &task_proxy, const Pattern &pattern) {
    vector<size_t> hash_multipliers;
    hash_multipliers.reserve(pattern.size());
    size_t num_states = 1;
    for (int pattern_var_id : pattern) {
        hash_multipliers.push_back(num_states);
        VariableProxy var = task_proxy.get_variables()[pattern_var_id];
        if (utils::is_product_within_limit(num_states, var.get_domain_size(),
                                           numeric_limits<int>::max())) {
            num_states *= var.get_domain_size();
        } else {
            cerr << "Given pattern is too large! (Overflow occurred): " << endl;
            cerr << pattern << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }
    return PerfectHashFunction(Pattern(pattern), num_states, move(hash_multipliers));
}

MatchTree build_match_tree(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const vector<AbstractOperator> &abstract_operators) {
    MatchTree match_tree(projection, hash_function);
    for (size_t op_id = 0; op_id < abstract_operators.size(); ++op_id) {
        const AbstractOperator &op = abstract_operators[op_id];
        match_tree.insert(op_id, op.get_preconditions());
    }
    return match_tree;
}

bool is_goal_state(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    size_t state_index) {
    for (const FactPair &abstract_goal : projection.get_abstract_goals()) {
        int pattern_var_id = abstract_goal.var;
        int var_id = projection.get_pattern()[pattern_var_id];
        VariableProxy var = projection.get_task_proxy().get_variables()[var_id];
        int val = hash_function.unrank(state_index, pattern_var_id, var.get_domain_size());
        if (val != abstract_goal.value) {
            return false;
        }
    }
    return true;
}

vector<int> compute_distances(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const vector<AbstractOperator> &regression_operators,
    const MatchTree &match_tree) {
    vector<int> distances;
    distances.reserve(hash_function.get_num_states());
    // first implicit entry: priority, second entry: index for an abstract state
    priority_queues::AdaptiveQueue<size_t> pq;

    // initialize queue
    for (size_t state_index = 0; state_index < hash_function.get_num_states(); ++state_index) {
        if (is_goal_state(projection, hash_function, state_index)) {
            pq.push(0, state_index);
            distances.push_back(0);
        } else {
            distances.push_back(numeric_limits<int>::max());
        }
    }

    // Dijkstra loop
    while (!pq.empty()) {
        pair<int, size_t> node = pq.pop();
        int distance = node.first;
        size_t state_index = node.second;
        if (distance > distances[state_index]) {
            continue;
        }

        // regress abstract_state
        vector<int> applicable_operator_ids;
        match_tree.get_applicable_operator_ids(state_index, applicable_operator_ids);
        for (int op_id : applicable_operator_ids) {
            const AbstractOperator &op = regression_operators[op_id];
            size_t predecessor = op.apply_to_state(state_index);
            int alternative_cost = distances[state_index] + op.get_cost();
            if (alternative_cost < distances[predecessor]) {
                distances[predecessor] = alternative_cost;
                pq.push(alternative_cost, predecessor);
            }
        }
    }
    return distances;
}

shared_ptr<PatternDatabase> generate_pdb(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    bool dump,
    const vector<int> &operator_costs) {
    task_properties::verify_no_axioms(task_proxy);
    task_properties::verify_no_conditional_effects(task_proxy);
    assert(operator_costs.empty() ||
           operator_costs.size() == task_proxy.get_operators().size());
    assert(utils::is_sorted_unique(pattern));

    utils::Timer timer;
    Projection projection(task_proxy, pattern);
    PerfectHashFunction hash_function = compute_hash_function(
        task_proxy, pattern);
    OperatorType operator_type(OperatorType::Regression);
    bool compute_op_id_mapping = false;
    AbstractOperators abstract_operators_factory(
        projection,
        hash_function,
        operator_type,
        operator_costs,
        compute_op_id_mapping);
    const vector<AbstractOperator> &regression_operators = abstract_operators_factory.get_regression_operators();
    MatchTree regression_match_tree = build_match_tree(
        projection,
        hash_function,
        regression_operators);
    vector<int> distances = compute_distances(
        projection,
        hash_function,
        regression_operators,
        regression_match_tree);

    if (dump)
        utils::g_log << "PDB construction time: " << timer << endl;

    return make_shared<PatternDatabase>(move(hash_function), move(distances));
}
}
