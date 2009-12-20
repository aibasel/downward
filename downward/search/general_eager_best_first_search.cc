#include "general_eager_best_first_search.h"

#include "globals.h"
#include "heuristic.h"
#include "successor_generator.h"

#include <cassert>
#include <cstdlib>
using namespace std;

GeneralEagerBestFirstSearch::GeneralEagerBestFirstSearch(bool reopen_closed):
    reopen_closed_nodes(reopen_closed),
    open_list(0), f_evaluator(0),
    expanded_states(0), reopened_states(0),
    evaluated_states(0), generated_states(0),
    lastjump_expanded_states(0), lastjump_reopened_states(0),
    lastjump_evaluated_states(0), lastjump_generated_states(0),
    lastjump_f_value(-1) {
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

    heuristics.push_back(heuristic);
    best_heuristic_values.push_back(-1);
}

void GeneralEagerBestFirstSearch::initialize() {
    g_search_space = &search_space;
    if (heuristics.size() > 1) {
        cout << "WARNING: currently only one heuristic allowed in general search; ";
        cout << "skipping additional heuristics." << endl;
    }
    //TODO children classes should output which kind of search
    cout << "Conducting best first search" <<
        (reopen_closed_nodes? " with" : " without") << " reopening closes nodes" << endl;

    assert(open_list != NULL);
    assert(heuristics.size() > 0);

    for (unsigned int i = 0; i < heuristics.size(); i++)
        heuristics[i]->evaluate(*g_initial_state);
	open_list->evaluate(0, false);
    evaluated_states++;

    if(open_list->is_dead_end()) {
        assert(open_list->dead_end_is_reliable());
        cout << "Initial state is a dead end." << endl;
    }
    else {
        for (unsigned int i = 0; i < heuristics.size(); i++) {
            initial_h_values.push_back(heuristics[i]->get_heuristic());
        }
        if (f_evaluator) {
            f_evaluator->evaluate(0,false);
            lastjump_f_value = f_evaluator->get_value();
            jump_statistics();
        }
        if(check_progress()) {
            report_progress();
        }
        SearchNode node = search_space.get_node(*g_initial_state);
        node.open_initial(initial_h_values[0]);

        open_list->insert(node.get_state_buffer());
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
    cout << "Initial state h value ";
    print_heuristic_values(initial_h_values);
    cout << "." << endl;

    cout << "Expanded " << expanded_states << " state(s)." << endl;
    cout << "Reopened " << reopened_states << " state(s)." << endl;
    cout << "Evaluated " << evaluated_states << " state(s)." << endl;
    cout << "Generated " << generated_states << " state(s)." << endl;

    if (f_evaluator) {
    cout << "Expanded until last jump: "
         << lastjump_expanded_states << " state(s)." << endl;
    cout << "Reopened until last jump: "
         << lastjump_reopened_states << " state(s)." << endl;
    cout << "Evaluated until last jump: "
         << lastjump_evaluated_states << " state(s)." << endl;
    cout << "Generated until last jump: "
         << lastjump_generated_states << " state(s)." << endl;
    }

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
            for (unsigned int i = 0; i < heuristics.size(); i++) {
                heuristics[i]->reach_state(node.get_state(), *op, succ_node.get_state());
                heuristics[i]->evaluate(succ_state);
            }
            evaluated_states++;
            bool dead_end = false;
            for (unsigned int i = 0; i < heuristics.size(); i++) {
                if(heuristics[i]->is_dead_end()
                   && heuristics[i]->dead_ends_are_reliable()) {
                    dead_end = true;
                    break;
                }
            }
            if (dead_end) {
                succ_node.mark_as_dead_end();
                continue;
            }
			int succ_h = heuristics[0]->get_heuristic();
            succ_node.open(succ_h, node, op);
			open_list->evaluate(succ_node.get_g(), false);
            open_list->insert(succ_node.get_state_buffer());

            if(check_progress()) {
                report_progress();
            }

        } else if(succ_node.get_g() > node.get_g() + op->get_cost()) {
            // We found a new cheapest path to an open or closed state.
            if (reopen_closed_nodes) {
                // if we reopen closed nodes, do that
                if(succ_node.is_closed()) {
                    /* TODO: Verify that the heuristic is inconsistent.
                     * Otherwise, this is a bug. This is a serious
                     * assertion because it can show that a heuristic that
                     * was thought to be consistent isn't. Therefore, it
                     * should be present also in release builds, so don't
                     * use a plain assert. */
                    reopened_states++;
                }
                succ_node.reopen(node, op);
                heuristics[0]->set_evaluator_value(succ_node.get_h());
				open_list->evaluate(succ_node.get_g(), false);

                open_list->insert(succ_node.get_state_buffer());
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
        if(open_list->empty()) {
            cout << "Completely explored state space -- no solution!" << endl;
            assert(false);
            exit(1); // fix segfault in release mode
            // TODO: Deal with this properly. step() should return
            //       failure.
        }
        State state(open_list->remove_min());
        SearchNode node = search_space.get_node(state);

        // If the node is closed, we do not reopen it, as our heuristic
        // is consistent.
        // TODO: check this
        if(!node.is_closed()) {
            node.close();
            assert(!node.is_dead_end());
			update_jump_statistic(node);
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
    print_heuristic_values(best_heuristic_values);
    cout << " [expanded " << expanded_states << " state(s)]" << endl;
}

void GeneralEagerBestFirstSearch::update_jump_statistic(const SearchNode& node) {
	if (f_evaluator) {
		heuristics[0]->set_evaluator_value(node.get_h());
		f_evaluator->evaluate(node.get_g(), false);
		int new_f_value = f_evaluator->get_value();
		if (new_f_value > lastjump_f_value) {
			lastjump_f_value = new_f_value;
			jump_statistics();
			lastjump_expanded_states = expanded_states;
			lastjump_reopened_states = reopened_states;
			lastjump_evaluated_states = evaluated_states;
			lastjump_generated_states = generated_states;
		}
	}
}

void GeneralEagerBestFirstSearch::print_heuristic_values(const vector<int>& values) const {
    for(int i = 0; i < values.size(); i++) {
        cout << values[i];
        if(i != values.size() - 1)
            cout << "/";
    }
}

void GeneralEagerBestFirstSearch::set_f_evaluator(ScalarEvaluator* eval) {
    f_evaluator = eval;
}

void GeneralEagerBestFirstSearch::set_open_list(OpenList<state_var_t *> *open) {
    open_list = open;
}
