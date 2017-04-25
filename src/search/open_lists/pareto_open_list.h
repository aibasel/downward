#ifndef OPEN_LISTS_PARETO_OPEN_LIST_H
#define OPEN_LISTS_PARETO_OPEN_LIST_H

#include "../open_list_factory.h"
#include "../option_parser_util.h"

namespace pareto_open_list {
class ParetoOpenListFactory : public OpenListFactory {
    Options options;
public:
    explicit ParetoOpenListFactory(const Options &options);
    virtual ~ParetoOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif
