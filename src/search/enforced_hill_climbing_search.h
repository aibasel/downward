#ifndef ENFORCED_HILL_CLIMBING_SEARCH_H
#define ENFORCED_HILL_CLIMBING_SEARCH_H

#include "search_engine.h"
#include "globals.h"
#include "search_space.h"
#include "search_node_info.h"
#include "operator.h"
#include "state.h"
#include "open_lists/open_list.h"
#include "g_evaluator.h"
#include "search_progress.h"
#include <vector>
#include <map>

using namespace std;

class Options;

typedef pair<state_var_t *, pair<int, const Operator * > > OpenListEntryEHC;

enum PreferredUsage {
    PRUNE_BY_PREFERRED, RANK_PREFERRED_FIRST,
    MAX_PREFERRED_USAGE
};

class EnforcedHillClimbingSearch : public SearchEngine {
protected:
    OpenList<OpenListEntryEHC> *open_list;
    GEvaluator *g_evaluator;

    Heuristic *heuristic;
    bool preferred_contains_eval;
    vector<Heuristic *> preferred_heuristics;
    bool use_preferred;
    PreferredUsage preferred_usage;

    State current_state;
    int current_h;
    int current_g;

    // statistics
    map<int, pair<int, int> > d_counts;
    int num_ehc_phases;
    int last_expanded;

    virtual void initialize();
    virtual int step();
    int ehc();
    void get_successors(const State &state, vector<const Operator *> &ops);
    void evaluate(const State &parent, const Operator *op, const State &state);
public:
    EnforcedHillClimbingSearch(const Options &opts);
    virtual ~EnforcedHillClimbingSearch();

    virtual void statistics() const;
};

#endif
