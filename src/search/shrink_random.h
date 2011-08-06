#ifndef SHRINK_RANDOM_H
#define SHRINK_RANDOM_H
#include "shrink_bucket_based.h"
#include <vector>

class ShrinkRandom : public ShrinkBucketBased {
public:
    ShrinkRandom();
    ~ShrinkRandom();
    void shrink(Abstraction &abs, int threshold, bool force = false);

    bool is_bisimulation() const;
    bool has_memory_limit() const;
    bool is_dfp() const;

    std::string description() const;
};


#endif
