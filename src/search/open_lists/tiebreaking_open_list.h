#ifndef OPEN_LISTS_TIEBREAKING_OPEN_LIST_H
#define OPEN_LISTS_TIEBREAKING_OPEN_LIST_H

#include "../open_list_factory.h"
#include "../option_parser_util.h"

namespace tiebreaking_open_list {
class TieBreakingOpenListFactory : public OpenListFactory {
    Options options;
public:
    explicit TieBreakingOpenListFactory(const Options &options);
    virtual ~TieBreakingOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif
