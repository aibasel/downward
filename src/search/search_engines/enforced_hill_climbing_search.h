#ifndef SEARCH_ENGINES_ENFORCED_HILL_CLIMBING_SEARCH_H
#define SEARCH_ENGINES_ENFORCED_HILL_CLIMBING_SEARCH_H

#include "../evaluation_context.h"
#include "../open_list.h"
#include "../search_engine.h"

#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace options {
class Options;
}

namespace enforced_hill_climbing_search {
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
    std::unique_ptr<EdgeOpenList> open_list;

    Heuristic *heuristic;
    std::vector<Heuristic *> preferred_operator_heuristics;
    std::set<Heuristic *> heuristics;
    bool use_preferred;
    PreferredUsage preferred_usage;

    EvaluationContext current_eval_context;
    int current_phase_start_g;

    // Statistics
    std::map<int, std::pair<int, int>> d_counts;
    int num_ehc_phases;
    int last_num_expanded;

    void insert_successor_into_open_list(
        const EvaluationContext &eval_context,
        int parent_g,
        OperatorID op_id,
        bool preferred);
    void expand(EvaluationContext &eval_context);
    void reach_state(
        const GlobalState &parent, OperatorID op_id,
        const GlobalState &state);
    SearchStatus ehc();

protected:
    virtual void initialize() override;
    virtual SearchStatus step() override;

public:
    explicit EnforcedHillClimbingSearch(const options::Options &opts);
    virtual ~EnforcedHillClimbingSearch() override;

    virtual void print_statistics() const override;
};
}

#endif
