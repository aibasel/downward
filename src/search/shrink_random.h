#ifndef SHRINK_RANDOM_H
#define SHRINK_RANDOM_H

class Options;

#include "shrink_bucket_based.h"

class ShrinkRandom : public ShrinkBucketBased {
protected:
    virtual void partition_into_buckets(
        const Abstraction &abs, std::vector<Bucket> &buckets) const;

    virtual std::string name() const;
public:
    ShrinkRandom(const Options &opts);
    virtual ~ShrinkRandom();
};

#endif
