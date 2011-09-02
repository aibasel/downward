#ifndef EAGER_SEARCH_H
#define EAGER_SEARCH_H

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
class Options;

class EagerSearch : public SearchEngine {
    // Search Behavior parameters
    bool reopen_closed_nodes; // whether to reopen closed nodes upon finding lower g paths
    bool do_pathmax; // whether to use pathmax correction
    bool use_multi_path_dependence;

    OpenList<state_var_t *> *open_list;
    ScalarEvaluator *f_evaluator;

protected:
    int step();
    pair<SearchNode, bool> fetch_next_node();
    bool check_goal(const SearchNode &node);
    void update_jump_statistic(const SearchNode &node);
    void print_heuristic_values(const vector<int> &values) const;
    void reward_progress();

    vector<Heuristic *> heuristics;
    vector<Heuristic *> preferred_operator_heuristics;
    vector<Heuristic *> estimate_heuristics;
    // TODO: in the long term this
    // should disappear into the open list

    virtual void initialize();

public:
    EagerSearch(const Options &opts);
    void statistics() const;

    void dump_search_space();
};

#endif
