#ifndef SHRINK_BUCKET_BASED_H
#define SHRINK_BUCKET_BASED_H
#include "shrink_strategy.h"
#include <vector>

/* A base class for shrink strategies that put the states in an ordered
   vector of buckets,
   and compute the new abstraction based on the order of the buckets using
   compute_abstraction(...).
   (e.g. ShrinkFH and ShrinkRandom)
*/

class ShrinkBucketBased : public ShrinkStrategy {
public:
    ShrinkBucketBased();
    virtual ~ShrinkBucketBased();
    virtual void shrink(
        Abstraction &abs, int threshold, bool force = false) = 0;

    virtual bool is_bisimulation() const = 0;
    virtual bool has_memory_limit() const = 0;
    virtual bool is_dfp() const = 0;

    std::string description() const = 0;
protected:
    void compute_abstraction(
        vector<vector<AbstractStateRef> > &buckets, 
        int target_size, 
        vector<slist<AbstractStateRef> > &collapsed_groups);

};


#endif
