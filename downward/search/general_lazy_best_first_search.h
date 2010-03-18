#ifndef GENERAL_LAZY_BEST_FIRST_SEARCH_H
#define GENERAL_LAZY_BEST_FIRST_SEARCH_H

#include <vector>

#include "closed_list.h"
#include "open_lists/open_list.h"
#include "search_engine.h"
#include "state.h"
#include "scalar_evaluator.h"
#include "search_space.h"
#include "search_progress.h"

class Heuristic;
class Operator;


typedef pair<state_var_t *, const Operator *> OpenListEntryLazy;

class GeneralLazyBestFirstSearch: public SearchEngine {
protected:
    // Search Behavior parameters
    bool reopen_closed_nodes; // whether to reopen closed nodes upon finding lower g paths
    int bound;
    enum {original, pref_first, shuffled} succ_mode;

    OpenList<OpenListEntryLazy> *open_list;
    vector<Heuristic *> heuristics;
    vector<Heuristic *> preferred_operator_heuristics;
    vector<Heuristic *> estimate_heuristics;

    State current_state;
    state_var_t *current_predecessor_buffer;
    const Operator *current_operator;
    int current_g;

    SearchProgress search_progress;

    virtual void initialize();
    virtual int step();

    void generate_successors();
    int fetch_next_state();

    void reward_progress();

    void set_open_list(OpenList<OpenListEntryLazy> *open);
    void get_successor_operators(vector<const Operator *> &ops);
public:

    GeneralLazyBestFirstSearch(bool reopen_closed);
    virtual ~GeneralLazyBestFirstSearch();

    virtual void add_heuristic(Heuristic *heuristic, bool use_estimates,
                                   bool use_preferred_operators);

    virtual void statistics() const;
    void set_bound(int b) {bound = b;}
    int get_bound() {return bound;}
};

#endif
