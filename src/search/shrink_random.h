#ifndef SHRINK_RANDOM_H
#define SHRINK_RANDOM_H

#include "shrink_bucket_based.h"

class ShrinkRandom : public ShrinkBucketBased {
protected:
    virtual void partition_into_buckets(
        const Abstraction &abs, std::vector<Bucket> &buckets) const;
public:
    ShrinkRandom();
    virtual ~ShrinkRandom();

    virtual std::string description() const;
};

#endif
