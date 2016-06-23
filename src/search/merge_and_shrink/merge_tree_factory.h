#ifndef MERGE_AND_SHRINK_MERGE_TREE_FACTORY_H
#define MERGE_AND_SHRINK_MERGE_TREE_FACTORY_H

#include "../options/options.h"

#include <memory>

class AbstractTask;

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeTree;

class MergeTreeFactory {
protected:
    options::Options options;
public:
    explicit MergeTreeFactory(const options::Options &options);
    virtual ~MergeTreeFactory() = default;
    virtual MergeTree *compute_merge_tree(
        std::shared_ptr<AbstractTask> task,
        FactoredTransitionSystem &fts) = 0;
    virtual void dump_options() const = 0;
};
}

#endif
