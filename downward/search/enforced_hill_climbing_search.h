#ifndef ENFORCED_HILL_CLIMBING_SEARCH_H_
#define ENFORCED_HILL_CLIMBING_SEARCH_H_

#include "search_engine.h"
#include "globals.h"
#include "search_space.h"
#include "search_node_info.h"
#include "operator.h"
#include "state.h"
#include "open-lists/open_list.h"
#include "g_evaluator.h"
#include <vector>
#include <map>

using namespace std;

typedef pair<State, pair<int, const Operator * > > OpenListEntryEHC;

enum {rank_preferred_first = 1, prune_by_preferred = 2};

class EnforcedHillClimbingSearch : public SearchEngine {
protected:
    OpenList<OpenListEntryEHC> *open_list;
    SearchSpace search_space;
    GEvaluator *g_evaluator;

    Heuristic *heuristic;
    bool preferred_contains_eval;
    vector<Heuristic *> preferred_heuristics;
    bool use_preferred;
    int preferred_usage;

    State current_state;
    int current_h;

    // statistics
    int expanded;
    int generated_states;
    int generated_ops;
    map<int, pair<int, int> > d_counts;
    int num_ehc_phases;
    int last_expanded;

    virtual void initialize();
    virtual int step();
    int ehc();
    void clear_open_list();
    void get_successors(const State &state, vector<const Operator *> &ops);
    void evaluate(const State &state);
public:
    EnforcedHillClimbingSearch();
    virtual ~EnforcedHillClimbingSearch();

    virtual void statistics() const;
    virtual void add_heuristic(Heuristic *h, bool use_estimates,
                               bool use_preferred_operators);
    void set_preferred_usage(int usage) {preferred_usage = usage;}
};

#endif
