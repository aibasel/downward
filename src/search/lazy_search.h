#ifndef LAZY_SEARCH_H
#define LAZY_SEARCH_H

#include <vector>

#include "open_lists/open_list.h"
#include "search_engine.h"
#include "state.h"
#include "scalar_evaluator.h"
#include "search_space.h"
#include "search_progress.h"

class Heuristic;
class Operator;
class Options;

typedef std::pair<StateID, const Operator *> OpenListEntryLazy;

class LazySearch : public SearchEngine {
protected:
    OpenList<OpenListEntryLazy> *open_list;

    // Search Behavior parameters
    bool reopen_closed_nodes; // whether to reopen closed nodes upon finding lower g paths
    bool randomize_successors;
    bool preferred_successors_first;

    std::vector<Heuristic *> heuristics;
    std::vector<Heuristic *> preferred_operator_heuristics;
    std::vector<Heuristic *> estimate_heuristics;

    State current_state;
    StateID current_predecessor_id;
    const Operator *current_operator;
    int current_g;
    int current_real_g;

    virtual void initialize();
    virtual int step();

    void generate_successors();
    int fetch_next_state();

    void reward_progress();

    void get_successor_operators(std::vector<const Operator *> &ops);
public:

    LazySearch(const Options &opts);
    virtual ~LazySearch();
    void set_pref_operator_heuristics(std::vector<Heuristic *> &heur);

    virtual void statistics() const;
};

#endif
