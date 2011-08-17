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
    virtual ~ShrinkFH();

    virtual void shrink(Abstraction &abs, int threshold, bool force = false) const;

    virtual bool is_bisimulation() const;
    virtual bool has_memory_limit() const;
    virtual bool is_dfp() const;

    virtual std::string description() const;
private:
    void ordered_buckets_use_vector(
        const Abstraction &abs,
        vector<Bucket> &result) const;

    void ordered_buckets_use_map(
        const Abstraction &abs,
        vector<Bucket> &buckets) const;

    const HighLow f_start;
    const HighLow h_start;
};


#endif
