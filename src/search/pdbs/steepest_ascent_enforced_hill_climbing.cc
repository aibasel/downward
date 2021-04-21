#include "steepest_ascent_enforced_hill_climbing.h"

#include "abstract_operator.h"
#include "match_tree.h"
#include "pattern_database.h"
#include "pattern_database_factory.h"

#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"

#include "../utils/logging.h"
#include "../utils/rng.h"

#include <queue>

using namespace std;

namespace pdbs {
struct SearchNode {
    size_t state_hash_index;
    int g;
    int cost;
    shared_ptr<SearchNode> predecessor;

    SearchNode(size_t state_hash_index, int g, int cost, shared_ptr<SearchNode> predecessor)
        : state_hash_index(state_hash_index), g(g), cost(cost), predecessor(predecessor) {
    }
};

static vector<OperatorID> get_cheapest_operators(
    const vector<AbstractOperator> &progression_operators,
    const MatchTree &progression_match_tree,
    const vector<OperatorID> &abstract_to_concrete_op_ids,
    size_t from_state,
    size_t to_state) {
    vector<int> applicable_op_ids;
    progression_match_tree.get_applicable_operator_ids(from_state, applicable_op_ids);
    int best_cost = numeric_limits<int>::max();
    vector<OperatorID> cheapest_op_ids;
    for (int op_id : applicable_op_ids) {
        const AbstractOperator &op = progression_operators[op_id];
        int op_cost = op.get_cost();
        if (op_cost > best_cost) {
            continue;
        }

        size_t successor = op.apply_to_state(from_state);
        if (successor == to_state) {
            if (op_cost < best_cost) {
                cheapest_op_ids.clear();
                best_cost = op_cost;
            }
            assert(op_cost == best_cost);
            cheapest_op_ids.push_back(abstract_to_concrete_op_ids[op_id]);
        }
    }
    return cheapest_op_ids;
}

static vector<vector<OperatorID>> extract_plan(
    const vector<AbstractOperator> &progression_operators,
    const MatchTree &progression_match_tree,
    const vector<OperatorID> &abstract_to_concrete_op_ids,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    bool compute_wildcard_plan,
    const shared_ptr<SearchNode> &goal_node) {
    vector<vector<OperatorID>> plan;
    shared_ptr<SearchNode> current_node = goal_node;
    while (current_node->predecessor) {
        vector<OperatorID> cheapest_operators =
            get_cheapest_operators(
                progression_operators,
                progression_match_tree,
                abstract_to_concrete_op_ids,
                current_node->predecessor->state_hash_index,
                current_node->state_hash_index);

        if (compute_wildcard_plan) {
            rng->shuffle(cheapest_operators);
            plan.push_back(move(cheapest_operators));
        } else {
            OperatorID random_op_id = *rng->choose(cheapest_operators);
            plan.emplace_back();
            plan.back().push_back(random_op_id);
        }
        current_node = current_node->predecessor;
    }
    reverse(plan.begin(), plan.end());
    return plan;
}

static vector<vector<OperatorID>> bfs_for_improving_state(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const vector<int> &distances,
    const vector<AbstractOperator> &progression_operators,
    const MatchTree &progression_match_tree,
    const vector<OperatorID> &abstract_to_concrete_op_ids,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    bool compute_wildcard_plan,
    utils::Verbosity verbosity,
    int f_star,
    shared_ptr<SearchNode> &start_node) {
    // Start node may have been used in earlier iteration, so we reset it here.
    start_node->cost = -1;
    start_node->predecessor = nullptr;
    queue<shared_ptr<SearchNode>> open;
    unordered_set<size_t> closed;
    closed.insert(start_node->state_hash_index);
    int h_start = distances[start_node->state_hash_index];
    open.push(move(start_node));
    while (true) {
        assert(!open.empty());
        shared_ptr<SearchNode> current_node = open.front();
        open.pop();
        size_t current_state = current_node->state_hash_index;
        if (verbosity >= utils::Verbosity::DEBUG) {
            utils::g_log << "current state: " << current_state << endl;
        }

        vector<int> applicable_ops;
        progression_match_tree.get_applicable_operator_ids(current_state, applicable_ops);
        rng->shuffle(applicable_ops);

        shared_ptr<SearchNode> best_improving_succ_node = nullptr;
        int best_h_so_far = h_start;
        for (int op_id : applicable_ops) {
            const AbstractOperator &op = progression_operators[op_id];
            size_t successor_state = op.apply_to_state(current_state);
            if (verbosity >= utils::Verbosity::DEBUG) {
                utils::g_log << "successor state: " << successor_state << endl;
            }
            if (!closed.count(successor_state)) {
                if (verbosity >= utils::Verbosity::DEBUG) {
                    utils::g_log << "is new" << endl;
                }
                int h_succ = distances[successor_state];
                int op_cost = op.get_cost();
                int g_succ = current_node->g + op_cost;
                int f_succ = g_succ + h_succ;
                if (f_succ == f_star) {
                    if (verbosity >= utils::Verbosity::DEBUG) {
                        utils::g_log << "has f* value" << endl;
                    }
                    shared_ptr<SearchNode> succ_node =
                        make_shared<SearchNode>(
                            successor_state, g_succ, op_cost, current_node);
                    assert(!closed.count(successor_state));

                    if (is_goal_state(projection, hash_function, successor_state)) {
                        h_succ = -1;
                    }
                    if (h_succ < best_h_so_far) {
                        best_h_so_far = h_succ;
                        best_improving_succ_node = succ_node;
                    }

                    closed.insert(successor_state);
                    open.push(succ_node);
                }
            } else {
                if (verbosity >= utils::Verbosity::DEBUG) {
                    utils::g_log << "is not new" << endl;
                }
            }
        }
        if (best_improving_succ_node) {
            start_node = best_improving_succ_node;
            return extract_plan(
                progression_operators, progression_match_tree,
                abstract_to_concrete_op_ids, rng, compute_wildcard_plan,
                best_improving_succ_node);
        }
    }
}

vector<vector<OperatorID>> steepest_ascent_enforced_hill_climbing(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const vector<int> &distances,
    const AbstractOperators &abstract_operators,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    bool compute_wildcard_plan,
    utils::Verbosity verbosity,
    size_t initial_state) {
    if (verbosity >= utils::Verbosity::DEBUG) {
        utils::g_log << "Running steepest ascent EHC" << endl;
    }

    const vector<AbstractOperator> &progression_operators = abstract_operators.get_progression_operators();
    const vector<OperatorID> &abstract_to_concrete_op_ids = abstract_operators.get_abstract_to_concrete_op_ids();
    assert(abstract_to_concrete_op_ids.size() == progression_operators.size());
    MatchTree progression_match_tree = build_match_tree(
        projection,
        hash_function,
        progression_operators);

    vector<vector<OperatorID>> plan;
    const int f_star = distances[initial_state];
    shared_ptr<SearchNode> current_node =
        make_shared<SearchNode>(initial_state, 0, -1, nullptr);
    while (!is_goal_state(projection, hash_function, current_node->state_hash_index)) {
        if (verbosity >= utils::Verbosity::DEBUG) {
            utils::g_log << "Current start state of iteration: "
                         << current_node->state_hash_index << endl;
        }
        // current_node will be set to the last node of the BFS, thus containing
        // the improving state for the next iteration, and the updated g-value.
        vector<vector<OperatorID>> plateau_plan =
            bfs_for_improving_state(
                projection, hash_function, distances, progression_operators,
                progression_match_tree, abstract_to_concrete_op_ids, rng,
                compute_wildcard_plan, verbosity, f_star, current_node);
        if (verbosity >= utils::Verbosity::DEBUG) {
            utils::g_log << "BFS wildcard plan to next improving state: " << endl;
            int step = 1;
            for (const vector<OperatorID> &equivalent_ops : plateau_plan) {
                utils::g_log << "step #" << step << endl;
                for (OperatorID op_id : equivalent_ops) {
                    utils::g_log << "op id: " << op_id << endl;
                }
                ++step;
            }
        }
        plan.insert(plan.end(),
                    make_move_iterator(plateau_plan.begin()),
                    make_move_iterator(plateau_plan.end()));
    }

    return plan;
}
}
