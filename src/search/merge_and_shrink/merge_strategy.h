#ifndef MERGE_AND_SHRINK_SHRINK_MERGE_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_MERGE_STRATEGY_H

#include <string>
#include <vector>

class Abstraction;

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
    // implementations of get_next should decrease remaining_merges by one
    // everytime they return a pair of abstractions which are merged next.
    virtual std::pair<int, int> get_next(const std::vector<Abstraction *> &all_abstractions) = 0;
    virtual std::string name() const = 0;
};

#endif
