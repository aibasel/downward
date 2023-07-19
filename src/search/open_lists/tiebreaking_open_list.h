#ifndef OPEN_LISTS_TIEBREAKING_OPEN_LIST_H
#define OPEN_LISTS_TIEBREAKING_OPEN_LIST_H

#include "../open_list_factory.h"

#include "../plugins/plugin.h"

namespace tiebreaking_open_list {
class TieBreakingOpenListFactory : public OpenListFactory {
    plugins::Options options;
    bool pref_only;
    int size;
    std::vector<std::shared_ptr<Evaluator>> evaluators;
    bool allow_unsafe_pruning;
public:
    explicit TieBreakingOpenListFactory(const plugins::Options &opts);
    explicit TieBreakingOpenListFactory(
            bool pref_only,
            std::vector<std::shared_ptr<Evaluator>> evaluators,
            bool allow_unsafe_pruning);
    virtual ~TieBreakingOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif
