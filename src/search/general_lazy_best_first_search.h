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

class GeneralLazyBestFirstSearch : public SearchEngine {
protected:
    OpenList<OpenListEntryLazy> *open_list;

    // Search Behavior parameters
    bool reopen_closed_nodes; // whether to reopen closed nodes upon finding lower g paths
    enum {original, pref_first, shuffled} succ_mode;

    vector<Heuristic *> heuristics;
    vector<Heuristic *> preferred_operator_heuristics;
    vector<Heuristic *> estimate_heuristics;

    State current_state;
    state_var_t *current_predecessor_buffer;
    const Operator *current_operator;
    int current_g;
    int current_real_g;

    void set_pref_operator_heuristics(vector<Heuristic *> &heur);
    virtual void initialize();
    virtual int step();

    void generate_successors();
    int fetch_next_state();

    void reward_progress();

    void get_successor_operators(vector<const Operator *> &ops);
public:

    GeneralLazyBestFirstSearch(const SearchEngineOptions &options,
                               OpenList<OpenListEntryLazy> *open,
                               bool reopen_closed);
    virtual ~GeneralLazyBestFirstSearch();

    virtual void statistics() const;

    static SearchEngine *create(const std::vector<std::string> &config,
                                int start, int &end, bool dry_run);
    static SearchEngine *create_greedy(const std::vector<std::string> &config,
                                       int start, int &end, bool dry_run);
    static SearchEngine *create_weighted_astar(
        const std::vector<std::string> &config, int start, int &end,
        bool dry_run);
};

#endif
