#ifndef ENFORCED_HILL_CLIMBING_SEARCH_H
#define ENFORCED_HILL_CLIMBING_SEARCH_H

#include "g_evaluator.h"
#include "global_operator.h"
#include "global_state.h"
#include "globals.h"
#include "search_engine.h"
#include "search_node_info.h"
#include "search_progress.h"
#include "search_space.h"

#include "open_lists/open_list.h"

#include <map>
#include <vector>

using namespace std;

class Options;

typedef pair<StateID, pair<int, const GlobalOperator * > > OpenListEntryEHC;

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

    GlobalState current_state;
    int current_h;
    int current_g;

    // statistics
    map<int, pair<int, int> > d_counts;
    int num_ehc_phases;
    int last_expanded;

    virtual void initialize();
    virtual SearchStatus step();
    SearchStatus ehc();
    void get_successors(const GlobalState &state, vector<const GlobalOperator *> &ops);
    void evaluate(const GlobalState &parent, const GlobalOperator *op, const GlobalState &state);
public:
    explicit EnforcedHillClimbingSearch(const Options &opts);
    virtual ~EnforcedHillClimbingSearch() override;

    virtual void print_statistics() const override;
};

#endif
