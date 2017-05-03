#include "lazy_search.h"

#include "../heuristic.h"
#include "../open_list_factory.h"
#include "../option_parser.h"

#include "../algorithms/ordered_set.h"
#include "../task_utils/successor_generator.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <algorithm>
#include <limits>
#include <vector>

using namespace std;

namespace lazy_search {
LazySearch::LazySearch(const Options &opts)
    : SearchEngine(opts),
      open_list(opts.get<shared_ptr<OpenListFactory>>("open")->
                create_edge_open_list()),
      reopen_closed_nodes(opts.get<bool>("reopen_closed")),
      randomize_successors(opts.get<bool>("randomize_successors")),
      preferred_successors_first(opts.get<bool>("preferred_successors_first")),
      rng(utils::parse_rng_from_options(opts)),
      current_state(state_registry.get_initial_state()),
      current_predecessor_id(StateID::no_state),
      current_operator(nullptr),
      current_g(0),
      current_real_g(0),
      current_eval_context(current_state, 0, true, &statistics) {
    /*
      We initialize current_eval_context in such a way that the initial node
      counts as "preferred".
    */
}

void LazySearch::set_pref_operator_heuristics(
    vector<Heuristic *> &heur) {
    preferred_operator_heuristics = heur;
}

void LazySearch::initialize() {
    cout << "Conducting lazy best first search, (real) bound = " << bound << endl;

    assert(open_list);
    set<Heuristic *> hset;
    open_list->get_involved_heuristics(hset);

    // Add heuristics that are used for preferred operators (in case they are
    // not also used in the open list).
    hset.insert(preferred_operator_heuristics.begin(),
                preferred_operator_heuristics.end());

    heuristics.assign(hset.begin(), hset.end());
    assert(!heuristics.empty());
    const GlobalState &initial_state = state_registry.get_initial_state();
    for (Heuristic *heuristic : heuristics) {
        heuristic->notify_initial_state(initial_state);
    }
}

vector<OperatorID> LazySearch::get_successor_operators(
    const ordered_set::OrderedSet<OperatorID> &preferred_operators) const {
    vector<OperatorID> applicable_operators;
    g_successor_generator->generate_applicable_ops(
        current_state, applicable_operators);

    if (randomize_successors) {
        rng->shuffle(applicable_operators);
    }

    if (preferred_successors_first) {
        ordered_set::OrderedSet<OperatorID> successor_operators;
        for (OperatorID op_id : preferred_operators) {
            successor_operators.insert(op_id);
        }
        for (OperatorID op_id : applicable_operators) {
            successor_operators.insert(op_id);
        }
        return successor_operators.pop_as_vector();
    } else {
        return applicable_operators;
    }
}

void LazySearch::generate_successors() {
    ordered_set::OrderedSet<OperatorID> preferred_operators =
        collect_preferred_operators(
            current_eval_context, preferred_operator_heuristics);
    if (randomize_successors) {
        preferred_operators.shuffle(*rng);
    }

    vector<OperatorID> successor_operators =
        get_successor_operators(preferred_operators);

    statistics.inc_generated(successor_operators.size());

    for (OperatorID op_id : successor_operators) {
        const GlobalOperator *op = &g_operators[op_id.get_index()];
        int new_g = current_g + get_adjusted_cost(*op);
        int new_real_g = current_real_g + op->get_cost();
        bool is_preferred = preferred_operators.contains(op_id);
        if (new_real_g < bound) {
            EvaluationContext new_eval_context(
                current_eval_context.get_cache(), new_g, is_preferred, nullptr);
            open_list->insert(new_eval_context, make_pair(current_state.get_id(), get_op_index_hacked(op)));
        }
    }
}

SearchStatus LazySearch::fetch_next_state() {
    if (open_list->empty()) {
        cout << "Completely explored state space -- no solution!" << endl;
        return FAILED;
    }

    EdgeOpenListEntry next = open_list->remove_min();

    current_predecessor_id = next.first;
    current_operator = &g_operators[next.second];
    GlobalState current_predecessor = state_registry.lookup_state(current_predecessor_id);
    assert(current_operator->is_applicable(current_predecessor));
    current_state = state_registry.get_successor_state(current_predecessor, *current_operator);

    SearchNode pred_node = search_space.get_node(current_predecessor);
    current_g = pred_node.get_g() + get_adjusted_cost(*current_operator);
    current_real_g = pred_node.get_real_g() + current_operator->get_cost();

    /*
      Note: We mark the node in current_eval_context as "preferred"
      here. This probably doesn't matter much either way because the
      node has already been selected for expansion, but eventually we
      should think more deeply about which path information to
      associate with the expanded vs. evaluated nodes in lazy search
      and where to obtain it from.
    */
    current_eval_context = EvaluationContext(current_state, current_g, true, &statistics);

    return IN_PROGRESS;
}

SearchStatus LazySearch::step() {
    // Invariants:
    // - current_state is the next state for which we want to compute the heuristic.
    // - current_predecessor is a permanent pointer to the predecessor of that state.
    // - current_operator is the operator which leads to current_state from predecessor.
    // - current_g is the g value of the current state according to the cost_type
    // - current_real_g is the g value of the current state (using real costs)


    SearchNode node = search_space.get_node(current_state);
    bool reopen = reopen_closed_nodes && !node.is_new() &&
                  !node.is_dead_end() && (current_g < node.get_g());

    if (node.is_new() || reopen) {
        StateID dummy_id = current_predecessor_id;
        // HACK! HACK! we do this because SearchNode has no default/copy constructor
        if (dummy_id == StateID::no_state) {
            const GlobalState &initial_state = state_registry.get_initial_state();
            dummy_id = initial_state.get_id();
        }
        GlobalState parent_state = state_registry.lookup_state(dummy_id);
        SearchNode parent_node = search_space.get_node(parent_state);

        if (current_operator) {
            for (Heuristic *heuristic : heuristics)
                heuristic->notify_state_transition(
                    parent_state, *current_operator, current_state);
        }
        statistics.inc_evaluated_states();
        if (!open_list->is_dead_end(current_eval_context)) {
            // TODO: Generalize code for using multiple heuristics.
            if (reopen) {
                node.reopen(parent_node, current_operator);
                statistics.inc_reopened();
            } else if (current_predecessor_id == StateID::no_state) {
                node.open_initial();
                if (search_progress.check_progress(current_eval_context))
                    print_checkpoint_line(current_g);
            } else {
                node.open(parent_node, current_operator);
            }
            node.close();
            if (check_goal_and_set_plan(current_state))
                return SOLVED;
            if (search_progress.check_progress(current_eval_context)) {
                print_checkpoint_line(current_g);
                reward_progress();
            }
            generate_successors();
            statistics.inc_expanded();
        } else {
            node.mark_as_dead_end();
            statistics.inc_dead_ends();
        }
        if (current_predecessor_id == StateID::no_state) {
            print_initial_h_values(current_eval_context);
        }
    }
    return fetch_next_state();
}

void LazySearch::reward_progress() {
    open_list->boost_preferred();
}

void LazySearch::print_checkpoint_line(int g) const {
    cout << "[g=" << g << ", ";
    statistics.print_basic_statistics();
    cout << "]" << endl;
}

void LazySearch::print_statistics() const {
    statistics.print_detailed_statistics();
    search_space.print_statistics();
}
}
