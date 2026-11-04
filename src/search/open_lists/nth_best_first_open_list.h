#ifndef OPEN_LISTS_NTH_BEST_FIRST_OPEN_LIST_H
#define OPEN_LISTS_NTH_BEST_FIRST_OPEN_LIST_H

#include "../open_list_factory.h"
#include "../plugins/options.h"


/*
  Open list indexed by a single int, using FIFO tie-breaking.

  Implemented as a map from int to deques.
*/

namespace nth_best_first_open_list {
class NthBestFirstOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit NthBestFirstOpenListFactory(const plugins::Options &options);
    virtual ~NthBestFirstOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif

