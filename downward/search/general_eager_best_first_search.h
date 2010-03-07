#ifndef GENERAL_EAGER_BEST_FIRST_SEARCH_H
#define GENERAL_EAGER_BEST_FIRST_SEARCH_H

#include <vector>

#include "open_lists/open_list.h"
#include "search_engine.h"
#include "search_space.h"
#include "state.h"
#include "timer.h"
#include "evaluator.h"
#include "search_progress.h"

class Heuristic;
class Operator;
class ScalarEvaluator;

class GeneralEagerBestFirstSearch : public SearchEngine {
    // Search Behavior parameters
    bool reopen_closed_nodes; // whether to reopen closed nodes upon finding lower g paths

    OpenList<state_var_t *> *open_list;
    ScalarEvaluator *f_evaluator;


	SearchProgress search_progress;
protected:
    int step();
    pair<SearchNode, bool> fetch_next_node();
    bool check_goal(const SearchNode &node);
	void update_jump_statistic(const SearchNode& node);
	void print_heuristic_values(const vector<int>& values) const;

    vector<Heuristic *> heuristics;
    vector<Heuristic *> preferred_operator_heuristics;
    vector<Heuristic *> estimate_heuristics;
    // TODO: in the long term this
    // should disappear into the open list

    virtual void initialize();
    void set_f_evaluator(ScalarEvaluator *eval);
    void set_open_list(OpenList<state_var_t *> *open);

public:
    GeneralEagerBestFirstSearch(bool reopen_closed);
    ~GeneralEagerBestFirstSearch();
    void add_heuristic(Heuristic *heuristic, bool use_estimates,
                               bool use_preferred_operators);
    void statistics() const;

    void dump_search_space();
};

#endif
