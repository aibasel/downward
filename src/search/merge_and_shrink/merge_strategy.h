#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_H

#include <string>
#include <vector>

class TransitionSystem;

class MergeStrategy {
protected:
    int remaining_merges;
    virtual void dump_strategy_specific_options() const = 0;
public:
    MergeStrategy();
    virtual ~MergeStrategy() {}

    void dump_options() const;

    bool done() const {
        return remaining_merges == 0;
    }
    // Implementations of get_next have to decrease remaining_merges by one
    virtual std::pair<int, int> get_next(const std::vector<TransitionSystem *> &all_transition_systems) = 0;
    virtual std::string name() const = 0;
};

#endif
