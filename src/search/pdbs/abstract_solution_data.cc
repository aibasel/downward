#include "abstract_solution_data.h"

#include "pattern_collection_generator_single_cegar.h" // TODO: only for utils::Verbosity

#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"

#include "../utils/hash.h"
#include "../utils/logging.h"
#include "../utils/rng.h"

#include <functional>
#include <limits>
#include <queue>

using namespace std;

namespace pdbs {
struct SearchNode {
    State state;
    std::size_t hash;
    int g;
    int cost;
    std::shared_ptr<SearchNode> predecessor;
    SearchNode(State state, std::size_t hash, int g, int cost, std::shared_ptr<SearchNode> predecessor)
        : state(std::move(state)), hash(hash), g(g), cost(cost), predecessor(predecessor) {
    }
};
}

namespace utils {
inline void feed(HashState &hash_state, const pdbs::SearchNode &search_node) {
    feed(hash_state, search_node.state);
}
}

namespace pdbs {
AbstractSolutionData::AbstractSolutionData(
        const shared_ptr<AbstractTask> &concrete_task,
        const Pattern &pattern,
        const shared_ptr<utils::RandomNumberGenerator> &rng,
        bool wildcard_plans,
        utils::Verbosity verbosity)
        : pdb(new PatternDatabase(TaskProxy(*concrete_task), pattern)),
          abstracted_task(concrete_task, pattern),
          abs_task_proxy(abstracted_task),
          wildcard_plans(wildcard_plans),
          is_solvable(true),
          solved(false) {
    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "Computing plan for PDB with pattern "
             << get_pattern() << endl;
    }

    if (pdb->get_value_abstracted(abs_task_proxy.get_initial_state())
        == numeric_limits<int>::max()) {
        is_solvable = false;
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "PDB with pattern " << get_pattern()
                 << " is unsolvable" << endl;
        }
        return;
    }

    successor_generator::SuccessorGenerator succ_gen(abs_task_proxy);
    vector<State> state_sequence =
        steepest_ascent_enforced_hillclimbing(succ_gen, rng, verbosity);
    turn_state_sequence_into_plan(succ_gen, rng, verbosity, state_sequence);
}

static void dump_state_sequence(const vector<State> &state_sequence) {
    string prefix = "";
    for (const State &state : state_sequence) {
        utils::g_log << prefix << state.get_unpacked_values();
        prefix = ", ";
    }
    utils::g_log << endl;
}

vector<State> AbstractSolutionData::extract_state_sequence(
    const SearchNode &goal_node) const {
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

vector<State> AbstractSolutionData::bfs_for_improving_state(
    const successor_generator::SuccessorGenerator &succ_gen,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const int f_star,
    shared_ptr<SearchNode> &start_node) const {
//    utils::g_log << "Running BFS with start state " << start_node->state.get_values() << endl;
    assert(start_node->cost == -1);
    assert(start_node->predecessor == nullptr);
    queue<shared_ptr<SearchNode>> open;
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

vector<State> AbstractSolutionData::steepest_ascent_enforced_hillclimbing(
    const successor_generator::SuccessorGenerator &succ_gen,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    utils::Verbosity verbosity) const {
    State start = abs_task_proxy.get_initial_state();
    start.unpack();
    size_t start_index = pdb->get_abstract_state_index(start);
    const int f_star = pdb->get_value_for_index(start_index);
    vector<State> state_sequence;
    state_sequence.push_back(start);
    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "Running EHC with start state " << start.get_unpacked_values() << endl;
    }

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
            bfs_for_improving_state(succ_gen, rng, f_star, start_node);
        if (verbosity >= utils::Verbosity::VERBOSE) {
            utils::g_log << "BFS state sequence to next improving state (adding all except first one): ";
            dump_state_sequence(bfs_state_sequence);
        }
        /* We do not add the first state since this equals the last state of
           the previous call to BFS, or the initial state, which we add
           manually. */
        for (size_t i = 1; i < bfs_state_sequence.size(); ++i) {
            State &state = bfs_state_sequence[i];
            state_sequence.push_back(move(state));
        }
    }
    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "EHC terminated. State sequence: ";
        dump_state_sequence(state_sequence);
    }
    return state_sequence;
}

void AbstractSolutionData::turn_state_sequence_into_plan(
    const successor_generator::SuccessorGenerator &succ_gen,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    utils::Verbosity verbosity,
    const vector<State> &state_sequence) {
    wildcard_plan.reserve(state_sequence.size() - 1);
    if (verbosity >= utils::Verbosity::VERBOSE) {
        utils::g_log << "Turning state seqeuence into plan..." << endl;
    }
    for (size_t i = 0; i < state_sequence.size() - 1; ++i) {
        const State &state = state_sequence[i];
        const State &successor = state_sequence[i + 1];
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
        if (wildcard_plans) {
            wildcard_plan.push_back(equivalent_ops);
        } else {
            OperatorID random_op_id = *rng->choose(equivalent_ops);
            wildcard_plan.emplace_back();
            wildcard_plan.back().push_back(random_op_id);
        }
    }

    if (verbosity >= utils::Verbosity::VERBOSE) {
        print_plan();
    }
}

OperatorID AbstractSolutionData::get_concrete_op_id_for_abs_op_id(
        OperatorID abs_op_id, const shared_ptr<AbstractTask> &parent_task) const {
    OperatorProxy abs_op = abs_task_proxy.get_operators()[abs_op_id];
    return abs_op.get_ancestor_operator_id(parent_task.get());
}

int AbstractSolutionData::compute_plan_cost() const {
    int sum_cost = 0;
    for (auto step : wildcard_plan) {
        // all equivalent ops have the same cost, so we just take the first op
        sum_cost += abs_task_proxy.get_operators()[step[0]].get_cost();
    }
    return sum_cost;
}

void AbstractSolutionData::print_plan() const {
    utils::g_log << "##### Plan for pattern " << get_pattern() << " #####" << endl;
    int i = 1;
    for (const auto &eqv_ops : wildcard_plan) {
        utils::g_log << "step #" << i << endl;
        for (OperatorID opid : eqv_ops) {
            OperatorProxy op = abs_task_proxy.get_operators()[opid];
            utils::g_log << op.get_name() << " " << op.get_cost() << endl;
        }
        ++i;
    }
    utils::g_log << "##### End of plan #####" << endl;
}

}
