#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_H

#include <memory>
#include <string>
#include <vector>

class AbstractTask;
class TransitionSystem;

class MergeStrategy {
protected:
    int remaining_merges;
    virtual void dump_strategy_specific_options() const = 0;
public:
    MergeStrategy();
    virtual ~MergeStrategy() {}
    virtual void initialize(std::shared_ptr<AbstractTask> task);
    bool initialized() const;
    bool done() const;
    void dump_options() const;

    // Implementations of get_next have to decrease remaining_merges by one
    virtual std::pair<int, int> get_next(const std::vector<TransitionSystem *> &all_transition_systems) = 0;
    virtual std::string name() const = 0;
};

#endif
