#ifndef OPEN_LISTS_TIEBREAKING_OPEN_LIST_H
#define OPEN_LISTS_TIEBREAKING_OPEN_LIST_H

#include "open_list.h"
#include "../evaluator.h"

#include <deque>
#include <map>
#include <vector>
#include <utility>

class ScalarEvaluator;
class Options;
class OptionParser;

template<class Entry>
class TieBreakingOpenList : public OpenList<Entry> {
    typedef std::deque<Entry> Bucket;

    std::map<const std::vector<int>, Bucket> buckets;
    int size;

    std::vector<ScalarEvaluator *> evaluators;
    bool allow_unsafe_pruning; // don't insert if main evaluator
    // says dead end, even if not reliably

    int dimension() const;

public:
    TieBreakingOpenList(const Options &opts);
    TieBreakingOpenList(const std::vector<ScalarEvaluator *> &evals,
                        bool preferred_only, bool unsafe_pruning);
    virtual ~TieBreakingOpenList() override;

    // open list interface
    virtual void insert(EvaluationContext &eval_context,
                        const Entry &entry) override;
    virtual Entry remove_min(std::vector<int> *key = 0) override;
    virtual bool empty() const override;
    virtual void clear() override;

    // tuple evaluator interface
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override;

    static OpenList<Entry> *_parse(OptionParser &parser);
};

#include "tiebreaking_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
