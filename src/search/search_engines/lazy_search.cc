#include "lazy_search.h"

#include "../open_list_factory.h"
#include "../option_parser.h"

#include "../algorithms/ordered_set.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
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
      current_operator_id(OperatorID::no_operator),
      current_g(0),
      current_real_g(0),
      current_eval_context(current_state, 0, true, &statistics) {
    /*
      We initialize current_eval_context in such a way that the initial node
      counts as "preferred".
    */
}

void LazySearch::set_preferred_operator_evaluators(
    vector<shared_ptr<Evaluator>> &evaluators) {
    preferred_operator_evaluators = evaluators;
}

void LazySearch::initialize() {
    cout << "Conducting lazy best first search, (real) bound = " << bound << endl;

    assert(open_list);
    set<Evaluator *> evals;
    open_list->get_path_dependent_evaluators(evals);

    // Add evaluators that are used for preferred operators (in case they are
    // not also used in the open list).
    for (const shared_ptr<Evaluator> &evaluator : preferred_operator_evaluators) {
        evaluator->get_path_dependent_evaluators(evals);
    }

    path_dependent_evaluators.assign(evals.begin(), evals.end());
    const GlobalState &initial_state = state_registry.get_initial_state();
    for (Evaluator *evaluator : path_dependent_evaluators) {
        evaluator->notify_initial_state(initial_state);
    }
}

vector<OperatorID> LazySearch::get_successor_operators(
    const ordered_set::OrderedSet<OperatorID> &preferred_operators) const {
    vector<OperatorID> applicable_operators;
    successor_generator.generate_applicable_ops(
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
    ordered_set::OrderedSet<OperatorID> preferred_operators;
    for (const shared_ptr<Evaluator> &preferred_operator_evaluator : preferred_operator_evaluators) {
        collect_preferred_operators(current_eval_context,
                                    preferred_operator_evaluator.get(),
                                    preferred_operators);
    }
    if (randomize_successors) {
        preferred_operators.shuffle(*rng);
    }

    vector<OperatorID> successor_operators =
        get_successor_operators(preferred_operators);

    statistics.inc_generated(successor_operators.size());

    for (OperatorID op_id : successor_operators) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        int new_g = current_g + get_adjusted_cost(op);
        int new_real_g = current_real_g + op.get_cost();
        bool is_preferred = preferred_operators.contains(op_id);
        if (new_real_g < bound) {
            EvaluationContext new_eval_context(
                current_eval_context.get_cache(), new_g, is_preferred, nullptr);
            open_list->insert(new_eval_context, make_pair(current_state.get_id(), op_id));
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
    current_operator_id = next.second;
    GlobalState current_predecessor = state_registry.lookup_state(current_predecessor_id);
    OperatorProxy current_operator = task_proxy.get_operators()[current_operator_id];
    assert(task_properties::is_applicable(current_operator, current_predecessor.unpack()));
    current_state = state_registry.get_successor_state(current_predecessor, current_operator);

    SearchNode pred_node = search_space.get_node(current_predecessor);
    current_g = pred_node.get_g() + get_adjusted_cost(current_operator);
    current_real_g = pred_node.get_real_g() + current_operator.get_cost();

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
        if (current_operator_id != OperatorID::no_operator) {
            assert(current_predecessor_id != StateID::no_state);
            GlobalState parent_state = state_registry.lookup_state(current_predecessor_id);
            for (Evaluator *evaluator : path_dependent_evaluators)
                evaluator->notify_state_transition(
                    parent_state, current_operator_id, current_state);
        }
        statistics.inc_evaluated_states();
        if (!open_list->is_dead_end(current_eval_context)) {
            // TODO: Generalize code for using multiple evaluators.
            if (current_predecessor_id == StateID::no_state) {
                node.open_initial();
                if (search_progress.check_progress(current_eval_context))
                    statistics.print_checkpoint_line(current_g);
            } else {
                GlobalState parent_state = state_registry.lookup_state(current_predecessor_id);
                SearchNode parent_node = search_space.get_node(parent_state);
                OperatorProxy current_operator = task_proxy.get_operators()[current_operator_id];
                if (reopen) {
                    node.reopen(parent_node, current_operator, get_adjusted_cost(current_operator));
                    statistics.inc_reopened();
                } else {
                    node.open(parent_node, current_operator, get_adjusted_cost(current_operator));
                }
            }
            node.close();
            if (check_goal_and_set_plan(current_state))
                return SOLVED;
            if (search_progress.check_progress(current_eval_context)) {
                statistics.print_checkpoint_line(current_g);
                reward_progress();
            }
            generate_successors();
            statistics.inc_expanded();
        } else {
            node.mark_as_dead_end();
            statistics.inc_dead_ends();
        }
        if (current_predecessor_id == StateID::no_state) {
            print_initial_evaluator_values(current_eval_context);
        }
    }
    return fetch_next_state();
}

void LazySearch::reward_progress() {
    open_list->boost_preferred();
}

void LazySearch::print_statistics() const {
    statistics.print_detailed_statistics();
    search_space.print_statistics();
}
}
