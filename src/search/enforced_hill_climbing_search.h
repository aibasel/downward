#ifndef ENFORCED_HILL_CLIMBING_SEARCH_H
#define ENFORCED_HILL_CLIMBING_SEARCH_H

#include "evaluation_context.h"
#include "search_engine.h"

#include "open_lists/open_list.h"

#include <map>
#include <set>
#include <utility>
#include <vector>

class GEvaluator;
class Options;

typedef std::pair<StateID, std::pair<int, const GlobalOperator * > > OpenListEntryEHC;

enum class PreferredUsage {
    PRUNE_BY_PREFERRED,
    RANK_PREFERRED_FIRST
};

/*
  Enforced hill-climbing with deferred evaluation.

  TODO: We should test if this lazy implementation really has any benefits over
  an eager one. We hypothesize that both versions need to evaluate and store
  the same states anyways.
*/
class EnforcedHillClimbingSearch : public SearchEngine {
    std::vector<const GlobalOperator *> get_successors(
        EvaluationContext &eval_context);
    void expand(EvaluationContext &eval_context, int d);
    void reach_state(
        const GlobalState &parent, const GlobalOperator &op,
        const GlobalState &state);
    SearchStatus ehc();

    OpenList<OpenListEntryEHC> *open_list;
    GEvaluator *g_evaluator;

    Heuristic *heuristic;
    std::vector<Heuristic *> preferred_operator_heuristics;
    std::set<Heuristic *> heuristics;
    bool use_preferred;
    PreferredUsage preferred_usage;

    EvaluationContext current_eval_context;

    // Statistics
    std::map<int, std::pair<int, int> > d_counts;
    int num_ehc_phases;
    int last_num_expanded;

protected:
    virtual void initialize() override;
    virtual SearchStatus step() override;

public:
    explicit EnforcedHillClimbingSearch(const Options &opts);
    virtual ~EnforcedHillClimbingSearch() override;

    virtual void print_statistics() const override;
};

#endif
