#ifndef OPEN_LISTS_LINEAR_WEIGHTED_TYPE_BASED_OPEN_LIST_H
#define OPEN_LISTS_LINEAR_WEIGHTED_TYPE_BASED_OPEN_LIST_H

#include "../open_list_factory.h"
#include "../plugins/options.h"

/*
  Lin-Type(h) in Kuroiwa & Beck (ICAPS2022).
  Instead of Type-based open list which selects a random h uniformly,
  select h with a probability that is linearly interpolated between max/min h.

  This is an implementation for multiple evaluators.
*/

namespace linear_weighted_type_based_open_list {
class LinearWeightedTypeBasedOpenListFactory : public OpenListFactory {
    plugins::Options options;
public:
    explicit LinearWeightedTypeBasedOpenListFactory(const plugins::Options &options);
    virtual ~LinearWeightedTypeBasedOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif


