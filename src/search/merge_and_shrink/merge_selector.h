#ifndef MERGE_AND_SHRINK_MERGE_SELECTOR_H
#define MERGE_AND_SHRINK_MERGE_SELECTOR_H

#include <memory>
#include <utility>

class AbstractTask;

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeSelector {
protected:
    virtual std::string name() const = 0;
    virtual void dump_specific_options() const {}
public:
    MergeSelector() = default;
    virtual ~MergeSelector() = default;
    virtual std::pair<int, int> select_merge(FactoredTransitionSystem &fts)
        const = 0;
    virtual void initialize(std::shared_ptr<AbstractTask> task) = 0;
    void dump_options() const;
};
}

#endif
