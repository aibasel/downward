#ifndef MERGE_AND_SHRINK_SHRINK_RANDOM_H
#define MERGE_AND_SHRINK_SHRINK_RANDOM_H

#include "shrink_bucket_based.h"

namespace options {
class Options;
}

namespace merge_and_shrink {
class ShrinkRandom : public ShrinkBucketBased {
protected:
    virtual void partition_into_buckets(
        const FactoredTransitionSystem &fts,
        int index,
        std::vector<Bucket> &buckets) const override;

    virtual std::string name() const override;
    void dump_strategy_specific_options() const override {}
public:
    explicit ShrinkRandom(const options::Options &opts);
    virtual ~ShrinkRandom() override;
};
}

#endif
