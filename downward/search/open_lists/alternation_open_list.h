#ifndef OPEN_LISTS_ALTERNATION_OPEN_LIST_H
#define OPEN_LISTS_ALTERNATION_OPEN_LIST_H

#include "../open_list_factory.h"

namespace alternation_open_list {
class AlternationOpenListFactory : public OpenListFactory {
    std::vector<std::shared_ptr<OpenListFactory>> sublists;
    int boost;
public:
    AlternationOpenListFactory(
        const std::vector<std::shared_ptr<OpenListFactory>> &sublists,
        int boost);

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif
