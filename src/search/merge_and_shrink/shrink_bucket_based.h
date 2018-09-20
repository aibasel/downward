#ifndef MERGE_AND_SHRINK_SHRINK_BUCKET_BASED_H
#define MERGE_AND_SHRINK_SHRINK_BUCKET_BASED_H

#include "shrink_strategy.h"

#include <memory>
#include <vector>

namespace options {
class OptionParser;
class Options;
}

namespace utils {
class RandomNumberGenerator;
}

namespace merge_and_shrink {
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
   (up to) one bucket "in the middle" is partially abstracted.
*/
class ShrinkBucketBased : public ShrinkStrategy {
protected:
    using Bucket = std::vector<int>;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

private:
    StateEquivalenceRelation compute_abstraction(
        const std::vector<Bucket> &buckets,
        int target_size) const;

protected:
    virtual std::vector<Bucket> partition_into_buckets(
        const TransitionSystem &ts,
        const Distances &Distances) const = 0;
public:
    explicit ShrinkBucketBased(const options::Options &opts);
    virtual ~ShrinkBucketBased() override = default;
    virtual StateEquivalenceRelation compute_equivalence_relation(
        const TransitionSystem &ts,
        const Distances &distances,
        int target_size) const override;
    static void add_options_to_parser(options::OptionParser &parser);
};
}

#endif
