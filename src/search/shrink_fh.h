#ifndef SHRINK_FH_H
#define SHRINK_FH_H
#include "shrink_strategy.h"
#include <vector>

class Options;

typedef vector<AbstractStateRef> Bucket;

enum HighLow {HIGH, LOW};

//replaces Shrink_{High,Low}_f_{High,Low}_h
class ShrinkFH : public ShrinkStrategy {
public:
    ShrinkFH(const Options &opts);
    ShrinkFH(HighLow fs, HighLow hs);
    void shrink(Abstraction &abs, int threshold, bool force = false);

    bool is_bisimulation();
    bool has_memory_limit();
    bool is_dfp();

    std::string description();
private:
    void ordered_buckets_use_vector(
        const Abstraction &abs,
        bool all_in_same_bucket,
        vector<Bucket> &result);

    HighLow f_start;
    HighLow h_start;
};


#endif
