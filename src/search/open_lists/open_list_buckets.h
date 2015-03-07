#ifndef OPEN_LISTS_OPEN_LIST_BUCKETS_H
#define OPEN_LISTS_OPEN_LIST_BUCKETS_H

#include "open_list.h"
#include "../scalar_evaluator.h"

#include <deque>
#include <utility>
#include <vector>

class Options;

template<class Entry>
class BucketOpenList : public OpenList<Entry> {
    typedef std::deque<Entry> Bucket;
    std::vector<Bucket> buckets;
    mutable int lowest_bucket;
    int size;

    ScalarEvaluator *evaluator;
    int last_evaluated_value;
    bool last_preferred;
protected:
    ScalarEvaluator *get_evaluator() {return evaluator; }

public:
    BucketOpenList(const Options &opts);
    virtual ~BucketOpenList() override;

    virtual void insert(EvaluationContext &eval_context,
                        const Entry &entry) override;
    virtual Entry remove_min(std::vector<int> *key = 0) override;
    virtual bool empty() const override;
    virtual void clear() override;

    virtual void evaluate(int g, bool preferred) override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override;

    static OpenList<Entry> *_parse(OptionParser &parser);
};

#include "open_list_buckets.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
