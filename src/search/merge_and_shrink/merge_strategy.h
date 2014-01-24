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
    virtual std::pair<int, int> get_next(const std::vector<Abstraction *> &all_abstractions) = 0;

    void dump_options() const;
    virtual std::string name() const = 0;
};

#endif
