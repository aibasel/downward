#include "general_lazy_best_first_search.h"
#include "heuristic.h"
#include "successor_generator.h"
#include "open_lists/standard_scalar_open_list.h"
#include "open_lists/alternation_open_list.h"
#include "open_lists/tiebreaking_open_list.h"
#include "g_evaluator.h"
#include "sum_evaluator.h"

#include <algorithm>
#include <limits>

GeneralLazyBestFirstSearch::GeneralLazyBestFirstSearch(bool reopen_closed):
    reopen_closed_nodes(reopen_closed), bound(numeric_limits<int>::max()), succ_mode(pref_first),
    current_state(*g_initial_state),
    current_predecessor_buffer(NULL), current_operator(NULL),
    current_g(0) {
}

GeneralLazyBestFirstSearch::~GeneralLazyBestFirstSearch() {
}

void GeneralLazyBestFirstSearch::set_open_list(OpenList<OpenListEntryLazy> *open) {
    open_list = open;
}

void GeneralLazyBestFirstSearch::initialize() {
    //TODO children classes should output which kind of search
    cout << "Conducting lazy best first search, bound = " << bound << endl;

    assert(open_list != NULL);
    assert(heuristics.size() > 0);
}

void GeneralLazyBestFirstSearch::add_heuristic(Heuristic *heuristic,
                      bool use_estimates,
                      bool use_preferred_operators) {
    assert(use_estimates || use_preferred_operators);
    if (use_estimates || use_preferred_operators) {
        heuristics.push_back(heuristic);
    }
    if(use_estimates) {
        estimate_heuristics.push_back(heuristic);
        search_progress.add_heuristic(heuristic);
    }
    if(use_preferred_operators) {
        preferred_operator_heuristics.push_back(heuristic);
    }
}

void GeneralLazyBestFirstSearch::get_successor_operators(
    vector<const Operator *> &ops) {
    vector<const Operator *> all_operators;
    vector<const Operator *> preferred_operators;

    g_successor_generator->generate_applicable_ops(
            current_state, all_operators);

    for (int i = 0; i < preferred_operator_heuristics.size(); i++) {
        Heuristic *heur = preferred_operator_heuristics[i];
        if (!heur->is_dead_end())
            heur->get_preferred_operators(preferred_operators);
    }

    if (succ_mode == pref_first) {
        for (int i = 0; i < preferred_operators.size(); i++) {
            if (!preferred_operators[i]->is_marked()) {
                ops.push_back(preferred_operators[i]);
                preferred_operators[i]->mark();
            }
        }

        for(int i = 0; i < all_operators.size(); i++)
            if (!all_operators[i]->is_marked())
                ops.push_back(all_operators[i]);
    } else {
        for (int i = 0; i < preferred_operators.size(); i++)
            if (!preferred_operators[i]->is_marked())
                preferred_operators[i]->mark();
        ops.swap(all_operators);
        if (succ_mode == shuffled)
            random_shuffle(ops.begin(), ops.end());
    }
}

void GeneralLazyBestFirstSearch::generate_successors() {
    vector<const Operator *> operators;
    get_successor_operators(operators);
    search_progress.inc_generated(operators.size());

    state_var_t *current_state_buffer =
        search_space.get_node(current_state).get_state_buffer();

    for (int i = 0; i < operators.size(); i++) {
        int new_g = current_g + operators[i]->get_cost();
        bool is_preferred = operators[i]->is_marked();
        if (is_preferred)
            operators[i]->unmark();
        if (new_g < bound) {
            open_list->evaluate(new_g, is_preferred);
            open_list->insert(
                make_pair(current_state_buffer, operators[i]));
        }
    }
}

int GeneralLazyBestFirstSearch::fetch_next_state() {
    if(open_list->empty()) {
        cout << "Completely explored state space -- no solution!" << endl;
        return FAILED;
    }

    OpenListEntryLazy next = open_list->remove_min();

    current_predecessor_buffer = next.first;
    current_operator = next.second;
    State current_predecessor(current_predecessor_buffer);
    current_state = State(current_predecessor, *current_operator);
    current_g = search_space.get_node(current_predecessor).get_g() +
                current_operator->get_cost();


    return IN_PROGRESS;
}

int GeneralLazyBestFirstSearch::step() {
    // Invariants:
    // - current_state is the next state for which we want to compute the heuristic.
    // - current_predecessor is a permanent pointer to the predecessor of that state.
    // - current_operator is the operator which leads to current_state from predecessor.
    // - current_g is the g value of the current state


    SearchNode node = search_space.get_node(current_state);
    bool reopen = reopen_closed_nodes && (current_g < node.get_g()) && !node.is_dead_end()  && !node.is_new();

    if(node.is_new() || reopen) {
        state_var_t *dummy_address = current_predecessor_buffer;
        // HACK! HACK! we do this because SearchNode has no default/copy constructor
        if (dummy_address == NULL) {
            dummy_address = const_cast<state_var_t *>(g_initial_state->get_buffer());
        }

        SearchNode parent_node = search_space.get_node(State(dummy_address));
        const State perm_state = node.get_state();

        for(int i = 0; i < heuristics.size(); i++) {
            if (current_operator != NULL) {
                heuristics[i]->reach_state(parent_node.get_state(), *current_operator, perm_state);
            }
            heuristics[i]->evaluate(current_state);
        }
        search_progress.inc_evaluated();
        open_list->evaluate(current_g, false);
        if(!open_list->is_dead_end()) {
            // We use the value of the first heuristic, because SearchSpace only
            // supported storing one heuristic value
            int h = heuristics[0]->get_value();
            if (reopen) {
                node.reopen(parent_node, current_operator);
                search_progress.inc_reopened();
            } else if (current_predecessor_buffer == NULL) {
                node.open_initial(h);
                search_progress.get_initial_h_values();
            } else {
                node.open(h, parent_node, current_operator);
            }
            node.close();
            if(check_goal_and_set_plan(current_state))
                return SOLVED;
            if (search_progress.check_h_progress(current_g)) {
                reward_progress();
            }
            generate_successors();
            search_progress.inc_expanded();
        } else {
            node.mark_as_dead_end();
        }
    }
    return fetch_next_state();
}

void GeneralLazyBestFirstSearch::reward_progress() {
    // Boost the "preferred operator" open lists somewhat whenever
    open_list->boost_preferred();
}

void GeneralLazyBestFirstSearch::statistics() const {
    search_progress.print_statistics();
}
