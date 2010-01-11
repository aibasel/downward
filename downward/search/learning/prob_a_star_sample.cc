#include "prob_a_star_sample.h"

#include "../globals.h"
#include "../heuristic.h"
#include "../successor_generator.h"
#include "../operator.h"
#include "../landmarks/landmarks_graph.h"
#include "../state.h"
#include "../open-lists/open_list.h"
#include "../search_engine.h"
#include "../search_space.h"
#include "../state.h"
#include "../timer.h"
#include "../evaluator.h"
#include <cassert>
#include <stdlib.h>


using namespace std;

ProbAStarSample::ProbAStarSample(Heuristic *h, int size = 100)
{
    min_training_set_size = size;
	add_heuristic(h, true, false);
	expand_all_prob = 0.1;
	exp = 0;
	gen = 0;
}

ProbAStarSample::~ProbAStarSample() {
}


int ProbAStarSample::collect() {
	search();
	return search_space.size();
}

int ProbAStarSample::step() {
	if (search_space.size() >= min_training_set_size) {
		return SOLVED;
	}
	int to_expand = -1;
	bool expand_all = false;;
	if (drand48() < expand_all_prob) {
		expand_all = true;
	}

	SearchNode node = fetch_next_node();
    State state = node.get_state();

    if(check_goal(node))
        return SOLVED;

    vector<const Operator *> applicable_ops;
    exp++;
    g_successor_generator->generate_applicable_ops(node.get_state(),
						   applicable_ops);
    gen = gen + applicable_ops.size();
    branching_factor = (double) gen / (double) exp;
    if (!expand_all) {
    	to_expand = rand() % applicable_ops.size();
    }

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
