#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_H

#include <utility>

namespace merge_and_shrink {
class FactoredTransitionSystem;

/*
  A merge strategy dictates the order in which transition systems of the
  factored transition system maintained by the merge-and-shrink heuristic
  should be merged.

  We distinguish three types of merge strategies: a stateless type, a
  precomputed type, and a third unspecified type, which does not fit
  either category.

  Stateless merge strategies: they do not store any information and determine
  the next merge soley based on the current factored transition system.

  Precomputed merge strategies: they are represented in the form of a merge
  tree (see class MergeTree). They return the next merge based on that
  precomputed tree. This requires that the actual merge performed always
  matches the one dictated by the precomputed strategy, i.e. it is mandatory
  that the merge tree maintained by the precomputed strategy remains
  synchronized with the current factored transition system.

  Special merge strategies: these do not fit either of the other categories
  and are usually a combination of existing stateless and/or precomputed merge
  strategies. For example, the SCCs merge strategy (Sievers et al, ICAPS 2016)
  needs to know which of the SCCs have been merged. While merging all variables
  within an SCC, it makes use of a stateless merge strategy or a merge tree,
  until that SCC has been entirely processed. There is currently no such merge
  strategy in the Fast Downward repository.

  NOTE: While stateless merge strategies have full control over the merge
  order, this is not true for the specific implementation of merge tree,
  because we always perform the next "left-most" merge in the merge tree.
  See also the documentation in merge_tree.h.
*/
class MergeStrategy {
protected:
    const FactoredTransitionSystem &fts;
public:
    explicit MergeStrategy(const FactoredTransitionSystem &fts);
    virtual ~MergeStrategy() = default;
    virtual std::pair<int, int> get_next() = 0;
};
}

#endif
