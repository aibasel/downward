#ifndef SHRINK_RANDOM_H
#define SHRINK_RANDOM_H
#include "shrink_bucket_based.h"
#include <vector>

class ShrinkRandom : public ShrinkBucketBased {
public:
    ShrinkRandom();
    virtual ~ShrinkRandom();
    virtual void shrink(Abstraction &abs, int threshold, bool force = false) const;

    virtual bool is_bisimulation() const;
    virtual bool has_memory_limit() const;
    virtual bool is_dfp() const;

    virtual std::string description() const;
};


#endif
