#ifndef SHRINK_FH_H
#define SHRINK_FH_H
#include "shrink_bucket_based.h"
#include <vector>

class Options;

enum HighLow {HIGH, LOW};

//replaces Shrink_{High,Low}_f_{High,Low}_h
class ShrinkFH : public ShrinkBucketBased {
public:
    ShrinkFH(const Options &opts);
    ShrinkFH(HighLow fs, HighLow hs);
    void shrink(Abstraction &abs, int threshold, bool force = false);

    bool is_bisimulation() const;
    bool has_memory_limit() const;
    bool is_dfp() const;

    std::string description() const;
private:
    void ordered_buckets_use_vector(
        const Abstraction &abs,
        vector<Bucket> &result);

    void ordered_buckets_use_map(
        const Abstraction &abs,
        vector<Bucket> &buckets);

    HighLow f_start;
    HighLow h_start;
};


#endif
