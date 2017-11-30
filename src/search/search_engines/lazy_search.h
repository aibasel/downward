#ifndef SEARCH_ENGINES_LAZY_SEARCH_H
#define SEARCH_ENGINES_LAZY_SEARCH_H

#include "../evaluation_context.h"
#include "../evaluator.h"
#include "../global_state.h"
#include "../open_list.h"
#include "../operator_id.h"
#include "../search_engine.h"
#include "../search_progress.h"
#include "../search_space.h"

#include <memory>
#include <vector>

class Heuristic;

namespace options {
class Options;
}

namespace lazy_search {
class LazySearch : public SearchEngine {
protected:
    std::unique_ptr<EdgeOpenList> open_list;

    // Search behavior parameters
    bool reopen_closed_nodes; // whether to reopen closed nodes upon finding lower g paths
    bool randomize_successors;
    bool preferred_successors_first;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    std::vector<Heuristic *> heuristics;
    std::vector<Heuristic *> preferred_operator_heuristics;
    std::vector<Heuristic *> estimate_heuristics;

    GlobalState current_state;
    StateID current_predecessor_id;
    OperatorID current_operator_id;
    int current_g;
    int current_real_g;
    EvaluationContext current_eval_context;

    virtual void initialize() override;
    virtual SearchStatus step() override;

    void generate_successors();
    SearchStatus fetch_next_state();

    void reward_progress();

    std::vector<OperatorID> get_successor_operators(
        const ordered_set::OrderedSet<OperatorID> &preferred_operators) const;

    // TODO: Move into SearchEngine?
    void print_checkpoint_line(int g) const;

public:
    explicit LazySearch(const options::Options &opts);
    virtual ~LazySearch() = default;

    void set_pref_operator_heuristics(std::vector<Heuristic *> &heur);

    virtual void print_statistics() const override;
};
}

#endif
