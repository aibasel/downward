#ifndef BEST_FIRST_SEARCH_H
#define BEST_FIRST_SEARCH_H

#include <vector>

#include "closed_list.h"
#include "open_lists/open_list.h"
#include "search_engine.h"
#include "state.h"

class Heuristic;
class Operator;

typedef pair<const State *, const Operator *> OpenListEntry;

struct OpenListInfo {
    OpenListInfo(Heuristic *heur, bool only_pref);
    Heuristic *heuristic;
    bool only_preferred_operators;
    OpenList<OpenListEntry> *open;
    int priority; // low value indicates high priority
};

class BestFirstSearchEngine : public SearchEngine {
    std::vector<Heuristic *> heuristics;
    std::vector<Heuristic *> preferred_operator_heuristics;
    std::vector<OpenListInfo> open_lists;
    ClosedList<State, const Operator *> closed_list;

    std::vector<int> best_heuristic_values;
    int generated_states;

    State current_state;
    const State *current_predecessor;
    const Operator *current_operator;

    bool is_dead_end();
    bool check_goal();
    bool check_progress();
    void report_progress();
    void reward_progress();
    void generate_successors(const State *parent_ptr);
    int fetch_next_state();
    OpenListInfo *select_open_queue();
protected:
    virtual void initialize();
    virtual int step();
public:
    BestFirstSearchEngine(const SearchEngineOptions &options);
    ~BestFirstSearchEngine();
    virtual void add_heuristic(Heuristic *heuristic, bool use_estimates,
                               bool use_preferred_operators);
    virtual void statistics() const;
    static SearchEngine *create(const vector<string> &config,
                                int start, int &end, bool dry_run);
};

#endif
