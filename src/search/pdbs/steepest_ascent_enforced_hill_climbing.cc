#include "steepest_ascent_enforced_hill_climbing.h"

#include "pattern_database.h"

#include "../task_proxy.h"

#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"

#include "../utils/logging.h"
#include "../utils/rng.h"

#include <queue>

using namespace std;

namespace pdbs {
struct SearchNode {
    State state;
    size_t hash;
    int g;
    int cost;
    shared_ptr<SearchNode> predecessor;

    SearchNode(State state, size_t hash, int g, int cost,
               shared_ptr<SearchNode> predecessor)
        : state(move(state)), hash(hash), g(g), cost(cost),
          predecessor(predecessor) {
    }
};
}

namespace utils {
inline void feed(HashState &hash_state, const pdbs::SearchNode &search_node) {
    feed(hash_state, search_node.state);
}
}

namespace pdbs {
static vector<OperatorID> get_cheapest_operators(
    const TaskProxy &abs_task_proxy,
    const successor_generator::SuccessorGenerator& succ_gen,
    const State &from, const State &to) {
    vector<OperatorID> applicable_ops;
    succ_gen.generate_applicable_ops(from, applicable_ops);
    int best_cost = numeric_limits<int>::max();
    vector<OperatorID> cheapest_ops;
    for (OperatorID op_id : applicable_ops) {
        OperatorProxy op = abs_task_proxy.get_operators()[op_id];
        int op_cost = op.get_cost();
        if (op_cost > best_cost) {
            continue;
        }

        State succ = from.get_unregistered_successor(op);
        if (succ == to) {
            if (op_cost < best_cost) {
                cheapest_ops.clear();
                best_cost = op_cost;
            }
            assert(op_cost == best_cost);
            cheapest_ops.push_back(op_id);
        }
    }
    return cheapest_ops;
}

static vector<vector<OperatorID>> extract_plan(
    const TaskProxy &abs_task_proxy,
    const successor_generator::SuccessorGenerator& succ_gen,
    shared_ptr<SearchNode> goal_node) {
    vector<vector<OperatorID>> plan;
    shared_ptr<SearchNode> current_node = goal_node;
    while(current_node->predecessor) {
        plan.push_back(get_cheapest_operators(
            abs_task_proxy, succ_gen, current_node->predecessor->state, current_node->state));
        current_node = current_node->predecessor;
    }
    return plan;
}

static vector<vector<OperatorID>> bfs_for_improving_state(
    const TaskProxy &abs_task_proxy,
    const successor_generator::SuccessorGenerator &succ_gen,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const PatternDatabase &pdb,
    int f_star,
    shared_ptr<SearchNode> &start_node) {
    // Start node may have been used in earlier iteration, so we reset it here
    start_node->cost = -1;
    start_node->predecessor = nullptr;
//    utils::g_log << "Running BFS with start state " << start_node->state.get_values() << endl;
    queue <shared_ptr<SearchNode>> open;
    utils::HashSet<size_t> closed;
    closed.insert(start_node->hash);
    int h_start = pdb.get_value_for_index(start_node->hash);
    open.push(move(start_node));
    while (true) {
        shared_ptr<SearchNode> current_node = open.front();
        open.pop();

        vector<OperatorID> applicable_ops;
        succ_gen.generate_applicable_ops(current_node->state, applicable_ops);
        rng->shuffle(applicable_ops);

        shared_ptr<SearchNode> best_improving_succ_node;
        int best_h_so_far = h_start;

        for (OperatorID op_id : applicable_ops) {
            OperatorProxy op = abs_task_proxy.get_operators()[op_id];
//            utils::g_log << "applying op " << op.get_name() << endl;
            State successor = current_node->state.get_unregistered_successor(op);
            size_t successor_index = pdb.get_abstract_state_index(successor);
            if (!closed.count(successor_index)) {
                int h_succ = pdb.get_value_for_index(successor_index);
                int op_cost = op.get_cost();
                int g_succ = current_node->g + op_cost;
                int f_succ = g_succ + h_succ;
                if (f_succ == f_star) {
//                    utils::g_log << "f* successor: " << successor.get_values() << endl;
                    shared_ptr<SearchNode> succ_node =
                        make_shared<SearchNode>(
                            move(successor), successor_index, g_succ, op_cost, current_node);
                    assert(!closed.count(successor_index));

                    if (task_properties::is_goal_state(abs_task_proxy, succ_node->state)) {
                        h_succ = -1;
                    }
                    if (h_succ < best_h_so_far) {
                        best_h_so_far = h_succ;
                        best_improving_succ_node = succ_node;
                    }

                    closed.insert(successor_index);
                    open.push(succ_node);
                }
            }
        }
        if (best_improving_succ_node) {
            start_node = best_improving_succ_node;
            return extract_plan(abs_task_proxy, succ_gen, best_improving_succ_node);
        }
    }
}

static void print_plan(const TaskProxy &abs_task_proxy,
                       const PatternDatabase &pdb,
                       const vector<vector<OperatorID>> &plan) {
    utils::g_log << "##### Plan for pattern " << pdb.get_pattern() << " #####" << endl;
    int i = 1;
    for (const auto &eqv_ops : plan) {
        utils::g_log << "step #" << i << endl;
        for (OperatorID opid : eqv_ops) {
            OperatorProxy op = abs_task_proxy.get_operators()[opid];
            utils::g_log << op.get_name() << " " << op.get_cost() << endl;
        }
        ++i;
    }
    utils::g_log << "##### End of plan #####" << endl;
}

vector<vector<OperatorID>> steepest_ascent_enforced_hillclimbing(
    const TaskProxy &abs_task_proxy,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const PatternDatabase &pdb,
    bool compute_wildcard_plan,
    utils::Verbosity verbosity) {
    vector<vector<OperatorID>> plan;
    State start = abs_task_proxy.get_initial_state();
    start.unpack();
    size_t start_index = pdb.get_abstract_state_index(start);
    const int f_star = pdb.get_value_for_index(start_index);
    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "Running EHC with start state " << start.get_unpacked_values() << endl;
    }

    successor_generator::SuccessorGenerator succ_gen(abs_task_proxy);
    shared_ptr<SearchNode> start_node
        = make_shared<SearchNode>(move(start), start_index, 0, -1, nullptr);
    while (!task_properties::is_goal_state(abs_task_proxy, start_node->state)) {
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "Current start state of iteration: " << start_node->state.get_unpacked_values() << endl;
        }
        // start_node will be set to the last node of the BFS, thus containing
        // the improving state for the next iteration, and the updated g-value.
        vector<vector<OperatorID>> plateau_plan =
            bfs_for_improving_state(abs_task_proxy, succ_gen, rng, pdb, f_star, start_node);
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "BFS wildcard plan to next improving state: ";
            print_plan(abs_task_proxy, pdb, plateau_plan);
        }
        plan.insert(plan.end(), plateau_plan.begin(), plateau_plan.end());
    }

    if (!compute_wildcard_plan) {
        for (vector<OperatorID> &plan_step : plan) {
            OperatorID random_op_id = *rng->choose(plan_step);
            plan_step.clear();
            plan_step.push_back(random_op_id);
        }
    }
    if (verbosity >= utils::Verbosity::VERBOSE) {
        print_plan(abs_task_proxy, pdb, plan);
    }
    return plan;
}
}
