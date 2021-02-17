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
static vector<State> extract_state_sequence(
    const SearchNode& goal_node) {
    vector<State> state_sequence;
    // Handle first state outside the loop because we need to copy.
    state_sequence.push_back(goal_node.state);
    shared_ptr<SearchNode> current_node = goal_node.predecessor;
    while (true) {
        state_sequence.push_back(move(current_node->state));
        if (current_node->predecessor) {
            assert(current_node->cost != -1);
            current_node = current_node->predecessor;
        } else {
            reverse(state_sequence.begin(), state_sequence.end());
            return state_sequence;
        }
    }
}

static vector<State> bfs_for_improving_state(
    const TaskProxy &abs_task_proxy,
    const successor_generator::SuccessorGenerator& succ_gen,
    const shared_ptr<utils::RandomNumberGenerator>& rng,
    shared_ptr<PatternDatabase> pdb,
    const int f_star,
    shared_ptr<SearchNode>& start_node) {
//    utils::g_log << "Running BFS with start state " << start_node->state.get_values() << endl;
    assert(start_node->cost == -1);
    assert(start_node->predecessor == nullptr);
    queue <shared_ptr<SearchNode>> open;
    utils::HashSet<size_t> closed;
    closed.insert(start_node->hash);
    int h_start = pdb->get_value_for_index(start_node->hash);
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
            size_t successor_index = pdb->get_abstract_state_index(successor);
            if (!closed.count(successor_index)) {
                int h_succ = pdb->get_value_for_index(successor_index);
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
            return extract_state_sequence(*best_improving_succ_node);
        }
    }
}

static void dump_state_sequence(const vector<State>& state_sequence) {
    string prefix = "";
    for (const State& state : state_sequence) {
        utils::g_log << prefix << state.get_unpacked_values();
        prefix = ", ";
    }
    utils::g_log << endl;
}

static vector<vector<OperatorID>> turn_state_sequence_into_plan(
    const TaskProxy &abs_task_proxy,
    const successor_generator::SuccessorGenerator& succ_gen,
    const shared_ptr<utils::RandomNumberGenerator>& rng,
    const vector<State>& state_sequence,
    bool compute_wildcard_plan,
    utils::Verbosity verbosity) {
    vector<vector<OperatorID>> plan;
    plan.reserve(state_sequence.size() - 1);
    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "Turning state seqeuence into plan..." << endl;
    }
    for (size_t i = 0; i < state_sequence.size() - 1; ++i) {
        const State& state = state_sequence[i];
        const State& successor = state_sequence[i + 1];
//        utils::g_log << "transition from " << state.get_values() << "to " << successor.get_values() << endl;
        assert(successor != state);
        int best_cost = numeric_limits<int>::max();
        vector<OperatorID> applicable_ops;
        succ_gen.generate_applicable_ops(state, applicable_ops);
        vector<OperatorID> equivalent_ops;
        for (OperatorID op_id : applicable_ops) {
            OperatorProxy op = abs_task_proxy.get_operators()[op_id];
//            utils::g_log << "consider op " << op.get_name() << endl;
            int op_cost = op.get_cost();
            if (op_cost > best_cost) {
//                utils::g_log << "too expensive" << endl;
                continue;
            }

            State succ = state.get_unregistered_successor(op);
            if (succ == successor) {
//                utils::g_log << "leads to right successor" << endl;
                if (op_cost < best_cost) {
//                    utils::g_log << "new best cost, updating" << endl;
                    equivalent_ops.clear();
                    best_cost = op_cost;
                }
                assert(op_cost == best_cost);
                equivalent_ops.push_back(op_id);
            }
        }
        if (compute_wildcard_plan) {
            plan.push_back(equivalent_ops);
        } else {
            OperatorID random_op_id = *rng->choose(equivalent_ops);
            plan.emplace_back();
            plan.back().push_back(random_op_id);
        }
    }
    return plan;
}

static void print_plan(const TaskProxy &abs_task_proxy,
                       shared_ptr<PatternDatabase> pdb,
                       const vector<vector<OperatorID>> &plan) {
    utils::g_log << "##### Plan for pattern " << pdb.get() << " #####" << endl;
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
    const shared_ptr<utils::RandomNumberGenerator>& rng,
    shared_ptr<PatternDatabase> pdb,
    bool compute_wildcard_plan,
    utils::Verbosity verbosity) {
    State start = abs_task_proxy.get_initial_state();
    start.unpack();
    size_t start_index = pdb->get_abstract_state_index(start);
    const int f_star = pdb->get_value_for_index(start_index);
    vector<State> state_sequence;
    state_sequence.push_back(start);
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
        // Reset start node to start a new BFS in this iteration.
        start_node->cost = -1;
        start_node->predecessor = nullptr;
        // start_node will be set to the last node of the BFS, thus containing
        // the improving state for the next iteration, and the updated g-value.
        vector<State> bfs_state_sequence =
            bfs_for_improving_state(abs_task_proxy, succ_gen, rng, pdb, f_star, start_node);
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "BFS state sequence to next improving state (adding all except first one): ";
            dump_state_sequence(bfs_state_sequence);
        }
        /* We do not add the first state since this equals the last state of
           the previous call to BFS, or the initial state, which we add
           manually. */
        for (size_t i = 1; i < bfs_state_sequence.size(); ++i) {
            State& state = bfs_state_sequence[i];
            state_sequence.push_back(move(state));
        }
    }
    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "EHC terminated. State sequence: ";
        dump_state_sequence(state_sequence);
    }

    vector<vector<OperatorID>> plan =
        turn_state_sequence_into_plan(abs_task_proxy, succ_gen, rng, state_sequence, compute_wildcard_plan, verbosity);

    if (verbosity >= utils::Verbosity::VERBOSE) {
        print_plan(abs_task_proxy, pdb, plan);
    }
    return plan;
}
}