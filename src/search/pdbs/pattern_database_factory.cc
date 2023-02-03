#include "pattern_database_factory.h"

#include "abstract_operator.h"
#include "match_tree.h"
#include "pattern_database.h"

#include "../algorithms/priority_queues.h"
#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/math.h"
#include "../utils/rng.h"
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
class PatternDatabaseFactory {
    Projection projection;
    vector<int> distances;
    vector<int> generating_op_ids;
    vector<vector<OperatorID>> wildcard_plan;

    /*
      Computes all abstract operators, builds the match tree (successor
      generator) and then does a Dijkstra regression search to compute
      all final h-values (stored in distances). operator_costs can
      specify individual operator costs for each operator for action
      cost partitioning. If left empty, default operator costs are used.
    */
    void create_pdb(
        const TaskProxy &task_proxy,
        const vector<int> &operator_costs,
        bool compute_plan,
        const shared_ptr<utils::RandomNumberGenerator> &rng,
        bool compute_wildcard_plan);

    /*
      For a given abstract state (given as index), the according values
      for each variable in the state are computed and compared with the
      given pairs of goal variables and values. Returns true iff the
      state is a goal state.
    */
    bool is_goal_state(
        int state_index,
        const vector<FactPair> &abstract_goals,
        const VariablesProxy &variables) const;
public:
    PatternDatabaseFactory(
        const TaskProxy &task_proxy,
        const Pattern &pattern,
        const vector<int> &operator_costs = vector<int>(),
        bool compute_plan = false,
        const shared_ptr<utils::RandomNumberGenerator> &rng = nullptr,
        bool compute_wildcard_plan = false);
    ~PatternDatabaseFactory() = default;

    shared_ptr<PatternDatabase> extract_pdb();

    vector<vector<OperatorID>> && extract_wildcard_plan() {
        return move(wildcard_plan);
    };
};

static Projection compute_projection(const TaskProxy &task_proxy, const Pattern &pattern) {
    task_properties::verify_no_axioms(task_proxy);
    task_properties::verify_no_conditional_effects(task_proxy);
    assert(utils::is_sorted_unique(pattern));

    vector<int> hash_multipliers;
    hash_multipliers.reserve(pattern.size());
    int num_states = 1;
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
    return Projection(Pattern(pattern), num_states, move(hash_multipliers));
}

PatternDatabaseFactory::PatternDatabaseFactory(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    const vector<int> &operator_costs,
    bool compute_plan,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    bool compute_wildcard_plan)
    : projection(compute_projection(task_proxy, pattern)) {
    assert(operator_costs.empty() ||
           operator_costs.size() == task_proxy.get_operators().size());
    create_pdb(task_proxy, operator_costs, compute_plan, rng, compute_wildcard_plan);
}

void PatternDatabaseFactory::create_pdb(
    const TaskProxy &task_proxy, const vector<int> &operator_costs,
    bool compute_plan, const shared_ptr<utils::RandomNumberGenerator> &rng,
    bool compute_wildcard_plan) {
    VariablesProxy variables = task_proxy.get_variables();
    vector<int> variable_to_index(variables.size(), -1);
    for (size_t i = 0; i < projection.get_pattern().size(); ++i) {
        variable_to_index[projection.get_pattern()[i]] = i;
    }

    // compute all abstract operators
    vector<AbstractOperator> operators;
    for (OperatorProxy op : task_proxy.get_operators()) {
        int op_cost;
        if (operator_costs.empty()) {
            op_cost = op.get_cost();
        } else {
            op_cost = operator_costs[op.get_id()];
        }
        build_abstract_operators(
            projection, op, op_cost, variable_to_index,
            task_proxy.get_variables(), operators);
    }

    // build the match tree
    MatchTree match_tree(task_proxy, projection);
    for (size_t op_id = 0; op_id < operators.size(); ++op_id) {
        const AbstractOperator &op = operators[op_id];
        match_tree.insert(op_id, op.get_regression_preconditions());
    }

    // compute abstract goal var-val pairs
    vector<FactPair> abstract_goals;
    for (FactProxy goal : task_proxy.get_goals()) {
        int var_id = goal.get_variable().get_id();
        int val = goal.get_value();
        if (variable_to_index[var_id] != -1) {
            abstract_goals.emplace_back(variable_to_index[var_id], val);
        }
    }

    distances.reserve(projection.get_num_abstract_states());
    // first implicit entry: priority, second entry: index for an abstract state
    priority_queues::AdaptiveQueue<int> pq;

    // initialize queue
    for (int state_index = 0; state_index < projection.get_num_abstract_states(); ++state_index) {
        if (is_goal_state(state_index, abstract_goals, variables)) {
            pq.push(0, state_index);
            distances.push_back(0);
        } else {
            distances.push_back(numeric_limits<int>::max());
        }
    }

    if (compute_plan) {
        /*
          If computing a plan during Dijkstra, we store, for each state,
          an operator leading from that state to another state on a
          strongly optimal plan of the PDB. We store the first operator
          encountered during Dijkstra and only update it if the goal distance
          of the state was updated. Note that in the presence of zero-cost
          operators, this does not guarantee that we compute a strongly
          optimal plan because we do not minimize the number of used zero-cost
          operators.
         */
        generating_op_ids.resize(projection.get_num_abstract_states());
    }

    // Dijkstra loop
    while (!pq.empty()) {
        pair<int, int> node = pq.pop();
        int distance = node.first;
        int state_index = node.second;
        if (distance > distances[state_index]) {
            continue;
        }

        // regress abstract_state
        vector<int> applicable_operator_ids;
        match_tree.get_applicable_operator_ids(state_index, applicable_operator_ids);
        for (int op_id : applicable_operator_ids) {
            const AbstractOperator &op = operators[op_id];
            int predecessor = state_index + op.get_hash_effect();
            int alternative_cost = distances[state_index] + op.get_cost();
            if (alternative_cost < distances[predecessor]) {
                distances[predecessor] = alternative_cost;
                pq.push(alternative_cost, predecessor);
                if (compute_plan) {
                    generating_op_ids[predecessor] = op_id;
                }
            }
        }
    }

    // Compute abstract plan
    if (compute_plan) {
        /*
          Using the generating operators computed during Dijkstra, we start
          from the initial state and follow the generating operator to the
          next state. Then we compute all operators of the same cost inducing
          the same abstract transition and randomly pick one of them to
          set for the next state. We iterate until reaching a goal state.
          Note that this kind of plan extraction does not uniformly at random
          consider all successor of a state but rather uses the arbitrarily
          chosen generating operator to settle on one successor state, which
          is biased by the number of operators leading to the same successor
          from the given state.
        */
        State initial_state = task_proxy.get_initial_state();
        initial_state.unpack();
        int current_state =
            projection.rank(initial_state.get_unpacked_values());
        if (distances[current_state] != numeric_limits<int>::max()) {
            while (!is_goal_state(current_state, abstract_goals, variables)) {
                int op_id = generating_op_ids[current_state];
                assert(op_id != -1);
                const AbstractOperator &op = operators[op_id];
                int successor_state = current_state - op.get_hash_effect();

                // Compute equivalent ops
                vector<OperatorID> cheapest_operators;
                vector<int> applicable_operator_ids;
                match_tree.get_applicable_operator_ids(successor_state, applicable_operator_ids);
                for (int applicable_op_id : applicable_operator_ids) {
                    const AbstractOperator &applicable_op = operators[applicable_op_id];
                    int predecessor = successor_state + applicable_op.get_hash_effect();
                    if (predecessor == current_state && op.get_cost() == applicable_op.get_cost()) {
                        cheapest_operators.emplace_back(applicable_op.get_concrete_op_id());
                    }
                }
                if (compute_wildcard_plan) {
                    rng->shuffle(cheapest_operators);
                    wildcard_plan.push_back(move(cheapest_operators));
                } else {
                    OperatorID random_op_id = *rng->choose(cheapest_operators);
                    wildcard_plan.emplace_back();
                    wildcard_plan.back().push_back(random_op_id);
                }

                current_state = successor_state;
            }
        }
        utils::release_vector_memory(generating_op_ids);
    }
}

bool PatternDatabaseFactory::is_goal_state(
    int state_index,
    const vector<FactPair> &abstract_goals,
    const VariablesProxy &variables) const {
    for (const FactPair &abstract_goal : abstract_goals) {
        int pattern_var_id = abstract_goal.var;
        int var_id = projection.get_pattern()[pattern_var_id];
        VariableProxy var = variables[var_id];
        int val = projection.unrank(state_index, pattern_var_id, var.get_domain_size());
        if (val != abstract_goal.value) {
            return false;
        }
    }
    return true;
}

shared_ptr<PatternDatabase> PatternDatabaseFactory::extract_pdb() {
    return make_shared<PatternDatabase>(
        move(projection),
        move(distances));
}

shared_ptr<PatternDatabase> compute_pdb(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    const vector<int> &operator_costs,
    const shared_ptr<utils::RandomNumberGenerator> &rng) {
    PatternDatabaseFactory pdb_factory(task_proxy, pattern, operator_costs, false, rng);
    return pdb_factory.extract_pdb();
}

tuple<shared_ptr<PatternDatabase>, vector<vector<OperatorID>>>
compute_pdb_and_wildcard_plan(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    const vector<int> &operator_costs,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    bool compute_wildcard_plan) {
    PatternDatabaseFactory pdb_factory(task_proxy, pattern, operator_costs, true, rng, compute_wildcard_plan);
    return {
               pdb_factory.extract_pdb(), pdb_factory.extract_wildcard_plan()
    };
}
}
