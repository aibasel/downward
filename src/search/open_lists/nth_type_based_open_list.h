#ifndef OPEN_LISTS_NTH_TYPE_BASED_OPEN_LIST_H
#define OPEN_LISTS_NTH_TYPE_BASED_OPEN_LIST_H

#include "../open_list_factory.h"
#include "../plugins/options.h"


namespace nth_type_based_open_list {
class NthTypeBasedOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit NthTypeBasedOpenListFactory(const plugins::Options &options);
    virtual ~NthTypeBasedOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif


