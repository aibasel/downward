#ifndef MERGE_AND_SHRINK_SHRINK_MERGE_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_MERGE_STRATEGY_H

#include <string>
#include <vector>

class Abstraction;

class MergeStrategy {
protected:
    virtual void dump_strategy_specific_options() const = 0;
public:
    MergeStrategy() {}
    virtual ~MergeStrategy() {}

    virtual bool done() const = 0;
    virtual void get_next(const std::vector<Abstraction *> &all_abstractions, std::pair<int, int> &next_indices) = 0;

    void dump_options() const;
    virtual std::string name() const = 0;
};

#endif
