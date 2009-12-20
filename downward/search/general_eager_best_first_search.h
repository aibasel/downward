#ifndef GENERAL_EAGER_BEST_FIRST_SEARCH_H
#define GENERAL_EAGER_BEST_FIRST_SEARCH_H

#include <vector>

#include "open-lists/open_list.h"
#include "search_engine.h"
#include "search_space.h"
#include "state.h"
#include "timer.h"
#include "evaluator.h"

class Heuristic;
class Operator;
class ScalarEvaluator;

class GeneralEagerBestFirstSearch : public SearchEngine {
protected:
    // Search Behavior parameters
    bool reopen_closed_nodes; // whether to reopen closed nodes upon finding lower g paths

    OpenList<state_var_t *> *open_list;
    ScalarEvaluator *f_evaluator;

    // used for statistics
    std::vector<int> best_heuristic_values;
	std::vector<int> initial_h_values;// h values of the initial state

    int expanded_states;// nr states for which successors were generated
    int reopened_states; // nr of *closed* states which we reopened
    int evaluated_states;// nr states for which h fn was computed
    int generated_states;// nr states created in total (plus those removed since already in close list)

    int lastjump_expanded_states;// same guy but at point where the last jump in the open list
    int lastjump_reopened_states;// occured (jump == f-value of the first node in the queue increases)
    int lastjump_evaluated_states;
    int lastjump_generated_states;

    int lastjump_f_value;//f value obtained in the last jump

    int step();
    SearchNode fetch_next_node();
    bool check_goal(const SearchNode &node);
    void jump_statistics() const;
	void update_jump_statistic(const SearchNode& node);
	void print_heuristic_values(const vector<int>& values) const;

    bool check_progress();
    void report_progress();

    SearchSpace search_space;
    std::vector<Heuristic *> heuristics;
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

#endif /* GENERAL_EAGER_BEST_FIRST_SEARCH_H */
