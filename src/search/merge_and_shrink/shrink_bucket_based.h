#ifndef MERGE_AND_SHRINK_SHRINK_BUCKET_BASED_H
#define MERGE_AND_SHRINK_SHRINK_BUCKET_BASED_H

#include "shrink_strategy.h"

#include <vector>

/* A base class for bucket-based shrink strategies.

   A bucket-based strategy partitions the states into an ordered
   vector of buckets, from low to high priority, and then abstracts
   them to a given target size according to the following rules:

   Repeat until we respect the target size:
       If any bucket still contains two states:
           Combine two random states from the non-singleton bucket
           with the lowest priority.
       Otherwise:
           Combine the two lowest-priority buckets.

   For the (usual) case where the target size is larger than the
   number of buckets, this works out in such a way that the
   high-priority buckets are not abstracted at all, the low-priority
   buckets are abstracted by combining all states in each bucket, and
   (up to) one bucket "in the middle" is partially abstracted. */

class Options;

class ShrinkBucketBased : public ShrinkStrategy {
protected:
    typedef std::vector<AbstractStateRef> Bucket;

private:
    void compute_abstraction(
        const std::vector<Bucket> &buckets,
        int target_size,
        EquivalenceRelation &equivalence_relation) const;

protected:
    virtual void partition_into_buckets(
        const Abstraction &abs, std::vector<Bucket> &buckets) const = 0;

public:
    ShrinkBucketBased(const Options &opts);
    virtual ~ShrinkBucketBased();

    virtual bool reduce_labels_before_shrinking() const;

    virtual void shrink(Abstraction &abs, int threshold,
                        bool force = false);
};

#endif
