#ifndef MERGE_AND_SHRINK_SHRINK_FH_H
#define MERGE_AND_SHRINK_SHRINK_FH_H

#include "shrink_bucket_based.h"

#include <vector>

class Options;

/*
  TODO/NOTE: The behaviour of this strategy in the cases where we
  need to merge across buckets (i.e., when the number of (f, h) pairs
  is larger than the number of abstract states) is a bit stupid.

  In such a case, it would probably be better to at least try to be
  *h*-preserving, e.g. by switching to a strategy that buckets by h,
  giving high h the lowest priority.

  To see how we can get disastrous results here, try

  $ make debug && ./downward-debug --search 'astar(mas(10000,shrink_strategy=shrink_fh(LOW,HIGH)))' < output

  Of course, LOW/HIGH are not very clever parameters, but that is not
  the point here. The init h value drops from 43 to 0 at the point
  where we must merge across buckets, and never improves above 0 again.

  With the default HIGH/LOW parameter setting, we drop from 43 to 36
  (and then recover to 37), which is of course better, but still not
  great.
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
    virtual std::string name() const;
    virtual void dump_strategy_specific_options() const;

    virtual void partition_into_buckets(const TransitionSystem &ts, std::vector<Bucket> &buckets) const;

public:
    ShrinkFH(const Options &opts);
    virtual ~ShrinkFH();
};

#endif
