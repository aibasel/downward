#ifndef MERGE_AND_SHRINK_SHRINK_RANDOM_H
#define MERGE_AND_SHRINK_SHRINK_RANDOM_H

#include "shrink_bucket_based.h"

class Options;

class ShrinkRandom : public ShrinkBucketBased {
protected:
    virtual void partition_into_buckets(const TransitionSystem &ts, std::vector<Bucket> &buckets) const;

    virtual std::string name() const;
    void dump_strategy_specific_options() const {}
public:
    explicit ShrinkRandom(const Options &opts);
    virtual ~ShrinkRandom();
};

#endif
