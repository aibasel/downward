#ifndef SEARCH_ENGINES_EAGER_SEARCH_H
#define SEARCH_ENGINES_EAGER_SEARCH_H

#include "../open_list.h"
#include "../search_engine.h"

#include <memory>
#include <vector>

class Evaluator;
class Heuristic;
class PruningMethod;

namespace options {
class Options;
}

namespace eager_search {
class EagerSearch : public SearchEngine {
    const bool reopen_closed_nodes;
    const bool use_multi_path_dependence;

    std::unique_ptr<StateOpenList> open_list;
    Evaluator *f_evaluator;

    std::vector<Heuristic *> heuristics;
    std::vector<Heuristic *> preferred_operator_heuristics;

    std::shared_ptr<PruningMethod> pruning_method;

    std::pair<SearchNode, bool> fetch_next_node();
    void start_f_value_statistics(EvaluationContext &eval_context);
    void update_f_value_statistics(const SearchNode &node);
    void reward_progress();
    void print_checkpoint_line(int g) const;

protected:
    virtual void initialize() override;
    virtual SearchStatus step() override;

public:
    explicit EagerSearch(const options::Options &opts);
    virtual ~EagerSearch() = default;

    virtual void print_statistics() const override;

    void dump_search_space() const;
};
}

#endif
