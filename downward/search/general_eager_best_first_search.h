/*
 * GeneralEagerBestFirstSearch.h
 *
 *  Created on: Oct 8, 2009
 *      Author: karpase
 */

#ifndef GENERALEAGERBESTFIRSTSEARCH_H_
#define GENERALEAGERBESTFIRSTSEARCH_H_

#include <vector>

#include "fh_open_list.h"
#include "search_engine.h"
#include "search_space.h"
#include "state.h"
#include "timer.h"

class Heuristic;
class Operator;

class GeneralEagerBestFirstSearch : public SearchEngine {

	// Search Behavior parameters
	int wg, wh; // weights for g and h
	int tb; // tie-breaker
	bool reopen_closed_nodes; // whether to reopen closed nodes upon finding lower g paths

	// wg = 0, wh = 1, reopen = false -> Greedy Best First Search
	// wg = 1, wh = 1, reopen = true  -> A*
	// wg = 1, wh = w, reopen = true  -> weighted A*

    std::vector<Heuristic *> heuristics;
    SearchSpace search_space;
    FhOpenList<state_var_t *> open_list;

    std::vector<int> best_heuristic_values;

    int initial_h_value;// h value of the initial state

    int expanded_states;// nr states for which successors were generated
    int reopened_states; // nr of *closed* states which we reopened
    int evaluated_states;// nr states for which h fn was computed
    int generated_states;// nr states created in total (plus those removed since already in close list)

    int lastjump_expanded_states;// same guy but at point where the last jump in the open list
    int lastjump_reopened_states;// occured (jump == f-value of the first node in the queue increases)
    int lastjump_evaluated_states;
    int lastjump_generated_states;

    int lastjump_f_value;//f value obtained in the last jump

    Timer timer;

    SearchNode fetch_next_node();
    bool check_goal(const SearchNode &node);
    void jump_statistics() const;
protected:
    virtual void initialize();
    virtual int step();

    int get_f(SearchNode& node);
    int get_tie_breaker(SearchNode& node);

    bool check_progress();
    void report_progress();
public:
	enum {fifo = 0, h = 1, high_g = 2, low_g = 3};
	GeneralEagerBestFirstSearch(int weight_g, int weight_h, int tie_breaker, bool reopen_closed);
    ~GeneralEagerBestFirstSearch();
    virtual void add_heuristic(Heuristic *heuristic, bool use_estimates,
                               bool use_preferred_operators);
    virtual void statistics() const;

    void dump_search_space();
};

#endif /* GENERALEAGERBESTFIRSTSEARCH_H_ */
