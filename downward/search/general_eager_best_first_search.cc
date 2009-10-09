/*
 * GeneralEagerBestFirstSearch.cpp
 *
 *  Created on: Oct 8, 2009
 *      Author: karpase
 */

#include "general_eager_best_first_search.h"

#include "globals.h"
#include "heuristic.h"
#include "successor_generator.h"

#include <cassert>
#include <cstdlib>
using namespace std;

GeneralEagerBestFirstSearch::GeneralEagerBestFirstSearch(
		int weight_g, int weight_h, int tie_breaker, bool reopen_closed):
		wg(weight_g), wh(weight_h), tb(tie_breaker), reopen_closed_nodes(reopen_closed) {

    expanded_states = 0;
    reopened_states = 0;
    evaluated_states = 0;
    generated_states = 0;

    lastjump_expanded_states = 0;
    lastjump_reopened_states = 0;
    lastjump_evaluated_states = 0;
    lastjump_generated_states = 0;

    initial_h_value = -1;
    lastjump_f_value = -1;
}

GeneralEagerBestFirstSearch::~GeneralEagerBestFirstSearch() {
}

// TODO: changes this to add_open_list,
// including type of open list, use of preferred operators, which heuristic to use, etc.
void GeneralEagerBestFirstSearch::add_heuristic(Heuristic *heuristic,
					  bool use_estimates,
					  bool use_preferred_operators) {

    if (!use_estimates && !use_preferred_operators) {
    	cerr << "WTF" << endl;
    }

    if (use_preferred_operators) {
    	cerr << "Preferred operator not supported" << endl;
    }

	assert(use_estimates || use_preferred_operators);

    if ( heuristics.size() > 0 ) {
      cout << "Warning: only one heuristic in A*; skipping." << endl;
    }
    else {
      heuristics.push_back(heuristic);
    }

    best_heuristic_values.push_back(-1);
}

void GeneralEagerBestFirstSearch::initialize() {
    cout << "Conducting best first search with" <<
		" wg = " << wg <<
		" wh = " << wh <<
		(reopen_closed_nodes? " with" : " without") << " reopening closes nodes" <<
		endl;

    assert(heuristics.size() == 1);

    heuristics[0]->evaluate(*g_initial_state);
    evaluated_states++;

    if(heuristics[0]->is_dead_end()) {
    	assert(heuristics[0]->dead_ends_are_reliable());
    	cout << "Initial state is a dead end." << endl;
    }
    else {
    	initial_h_value = heuristics[0]->get_heuristic();
    	lastjump_f_value = 0 + initial_h_value;
        jump_statistics();
        SearchNode node = search_space.get_node(*g_initial_state);
        node.open_initial(initial_h_value);

        open_list.insert(get_f(node), get_tie_breaker(node), node.get_state_buffer());
    }
}


void GeneralEagerBestFirstSearch::jump_statistics() const {
    cout << "f = " << lastjump_f_value
         << " [" << evaluated_states << " evaluated, "
         << expanded_states << " expanded, ";
    if(reopened_states) {
        /* TODO: Replace this test by the test if the heuristic is
         consistent. (Each heuristic should know if it is.) */
        cout << reopened_states << " reopened, ";
    }
    cout << "t=" << g_timer << "]" << endl;
}

void GeneralEagerBestFirstSearch::statistics() const {
    cout << "Initial state h value " << initial_h_value << "." << endl;

    cout << "Expanded " << expanded_states << " state(s)." << endl;
    cout << "Reopened " << reopened_states << " state(s)." << endl;
    cout << "Evaluated " << evaluated_states << " state(s)." << endl;
    cout << "Generated " << generated_states << " state(s)." << endl;

    cout << "Expanded until last jump: "
         << lastjump_expanded_states << " state(s)." << endl;
    cout << "Reopened until last jump: "
         << lastjump_reopened_states << " state(s)." << endl;
    cout << "Evaluated until last jump: "
         << lastjump_evaluated_states << " state(s)." << endl;
    cout << "Generated until last jump: "
         << lastjump_generated_states << " state(s)." << endl;

    search_space.statistics();
}

int GeneralEagerBestFirstSearch::step() {
    SearchNode node = fetch_next_node();

    if(check_goal(node))
        return SOLVED;

    vector<const Operator *> applicable_ops;
    g_successor_generator->generate_applicable_ops(node.get_state(),
						   applicable_ops);
    for(int i = 0; i < applicable_ops.size(); i++) {
	const Operator *op = applicable_ops[i];
	State succ_state(node.get_state(), *op);
	generated_states++;

	SearchNode succ_node = search_space.get_node(succ_state);

	if(succ_node.is_dead_end()) {
	    // Previously encountered dead end. Don't re-evaluate.
	    continue;
	} else if(succ_node.is_new()) {
	    // We have not seen this state before.
	    // Evaluate and create a new node.
		heuristics[0]->reach_state(node.get_state(), *op, succ_node.get_state());
	    heuristics[0]->evaluate(succ_state);
	    evaluated_states++;
	    if(heuristics[0]->is_dead_end()) {
		assert(heuristics[0]->dead_ends_are_reliable());
		succ_node.mark_as_dead_end();
		continue;
	    }
	    int succ_h = heuristics[0]->get_heuristic();
	    succ_node.open(succ_h, node, op);
	    open_list.insert(get_f(succ_node), get_tie_breaker(succ_node),
			     succ_node.get_state_buffer());

	    if(check_progress()) {
	  		report_progress();
	  	}

	} else if(succ_node.get_g() > node.get_g() + op->get_cost()) {
            // We found a new cheapest path to an open or closed state.
			if (reopen_closed_nodes) {
				// if we reopen closed nodes, do that
				if(succ_node.is_closed()) {
					/* TODO: Verify that the heuristic is inconsistent.
					   Otherwise, this is a bug. This is a serious
					   assertion because it can show that a heuristic that
					   was thought to be consistent isn't. Therefore, it
					   should be present also in release builds, so don't
					   use a plain assert. */
					reopened_states++;
				}
				succ_node.reopen(node, op);
				open_list.insert(get_f(succ_node), get_tie_breaker(succ_node),
								 succ_node.get_state_buffer());
			}
			else {
				// if we do not reopen closed nodes, we just update the parent pointers
				succ_node.update_parent(node, op);
			}
	}
    }

    return IN_PROGRESS;
}

SearchNode GeneralEagerBestFirstSearch::fetch_next_node() {
    while(true) {
        if(open_list.empty()) {
            cout << "Completely explored state space -- no solution!" << endl;
            assert(false);
            exit(1); // fix segfault in release mode
            // TODO: Deal with this properly. step() should return
            //       failure.
        }
        State state(open_list.remove_min());
        SearchNode node = search_space.get_node(state);

        // If the node is closed, we do not reopen it, as our heuristic
        // is consistent.
        // TODO: check this
        if(!node.is_closed()) {
            node.close();
            assert(!node.is_dead_end());

            int new_f_value = get_f(node);
            if (new_f_value > lastjump_f_value) {
                lastjump_f_value = new_f_value;
                jump_statistics();
                lastjump_expanded_states = expanded_states;
                lastjump_reopened_states = reopened_states;
                lastjump_evaluated_states = evaluated_states;
                lastjump_generated_states = generated_states;
            }
            expanded_states++;
            return node;
        }
    }
}

bool GeneralEagerBestFirstSearch::check_goal(const SearchNode &node) {
    if (node.is_goal()) {
        cout << "Solution found!" << endl;
        Plan plan;
        search_space.trace_path(node.get_state(), plan);
        set_plan(plan);
        return true;
    }
    return false;
}

void GeneralEagerBestFirstSearch::dump_search_space()
{
  search_space.dump();
}

int GeneralEagerBestFirstSearch::get_f(SearchNode& node) {
	return (wg * node.get_g()) + (wh * node.get_h());
}

int GeneralEagerBestFirstSearch::get_tie_breaker(SearchNode& node) {
	switch (tb) {
	case fifo:
		return 0;
	case h:
		return node.get_h();
	case high_g:
		// todo: make sure this would work with -1 * g and change it
		return (10000 - node.get_g());
	case low_g:
		return node.get_g();
	}
	return 0;
}

bool GeneralEagerBestFirstSearch::check_progress() {
    bool progress = false;
    for(int i = 0; i < heuristics.size(); i++) {
	if(heuristics[i]->is_dead_end())
	    continue;
	int h = heuristics[i]->get_heuristic();
	int &best_h = best_heuristic_values[i];
	if(best_h == -1 || h < best_h) {
	    best_h = h;
	    progress = true;
	}
    }
    return progress;
}

void GeneralEagerBestFirstSearch::report_progress() {
    cout << "Best heuristic value: ";
    for(int i = 0; i < heuristics.size(); i++) {
	cout << best_heuristic_values[i];
	if(i != heuristics.size() - 1)
	    cout << "/";
    }
    cout << " [expanded " << expanded_states << " state(s)]" << endl;
}
