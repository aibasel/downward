#ifndef OPEN_LISTS_BUCKET_OPEN_LIST_H
#define OPEN_LISTS_BUCKET_OPEN_LIST_H

#include "open_list.h"

#include <deque>
#include <vector>

class Options;
class ScalarEvaluator;

/*
  Bucket-based implementation of an open list.

  Nodes with identical heuristic value are expanded in FIFO order.
*/

template<class Entry>
class BucketOpenList : public OpenList<Entry> {
    typedef std::deque<Entry> Bucket;
    std::vector<Bucket> buckets;
    mutable int lowest_bucket;
    int size;

    ScalarEvaluator *evaluator;

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    explicit BucketOpenList(const Options &opts);
    virtual ~BucketOpenList() override = default;

    virtual Entry remove_min(std::vector<int> *key = 0) override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;

    static OpenList<Entry> *_parse(OptionParser &parser);
};

#include "bucket_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
