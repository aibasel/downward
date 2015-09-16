#ifndef MERGE_AND_SHRINK_SHRINK_FH_H
#define MERGE_AND_SHRINK_SHRINK_FH_H

#include "shrink_bucket_based.h"

#include <vector>

class Options;

/*
  NOTE: In case where we must merge across buckets (i.e. when
  the number of (f, h) pairs is larger than the number of
  permitted abstract states), this shrink strategy will *not* make
  an effort to be at least be h-preserving.

  This could be improved, but not without complicating the code.
  Usually we set the number of abstract states large enough that we
  do not need to merge across buckets. Therefore the complication
  might not be worth the code maintenance cost.
*/

class ShrinkFH : public ShrinkBucketBased {
public:
    enum HighLow {HIGH, LOW};

private:
    const HighLow f_start;
    const HighLow h_start;

    void ordered_buckets_use_vector(const TransitionSystem &ts,
                                    std::vector<Bucket> &buckets) const;
    void ordered_buckets_use_map(const TransitionSystem &ts,
                                 std::vector<Bucket> &buckets) const;
protected:
    virtual std::string name() const override;
    virtual void dump_strategy_specific_options() const override;

    virtual void partition_into_buckets(
        const TransitionSystem &ts, std::vector<Bucket> &buckets) const;

public:
    explicit ShrinkFH(const Options &opts);
    virtual ~ShrinkFH();
};

#endif
