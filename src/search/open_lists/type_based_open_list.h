#ifndef OPEN_LISTS_TYPE_BASED_OPEN_LIST_H
#define OPEN_LISTS_TYPE_BASED_OPEN_LIST_H

#include "../open_list_factory.h"

/*
  Type-based open list based on Xie et al. (AAAI 2014; see detailed
  reference in plug-in documentation).

  The original implementation uses a std::map for storing and looking
  up buckets. Our implementation stores the buckets in a std::vector
  and uses a std::unordered_map for looking up indexes in the vector.

  In the table below we list the amortized worst-case time complexities
  for the original implementation and the version below.

    n = number of entries
    m = number of buckets

                            Original    Code below
    Insert entry            O(log(m))   O(1)
    Remove entry            O(m)        O(1)        # both use swap+pop
*/

namespace type_based_open_list {
class TypeBasedOpenListFactory : public OpenListFactory {
    std::vector<std::shared_ptr<Evaluator>> evaluators;
    int random_seed;
public:
    TypeBasedOpenListFactory(
        const std::vector<std::shared_ptr<Evaluator>> &evaluators,
        int random_seed);

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif
