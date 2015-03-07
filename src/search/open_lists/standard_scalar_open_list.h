#ifndef OPEN_LISTS_STANDARD_SCALAR_OPEN_LIST_H
#define OPEN_LISTS_STANDARD_SCALAR_OPEN_LIST_H

#include "open_list.h"

#include <deque>
#include <map>
#include <vector>
#include <utility>

class Options;
class OptionParser;
class ScalarEvaluator;

template<class Entry>
class StandardScalarOpenList : public OpenList<Entry> {
    typedef std::deque<Entry> Bucket;

    std::map<int, Bucket> buckets;
    int size;

    ScalarEvaluator *evaluator;
    int last_evaluated_value;
    int last_preferred;
protected:
    virtual ScalarEvaluator *get_evaluator() override {return evaluator; }

public:
    StandardScalarOpenList(const Options &opts);
    StandardScalarOpenList(ScalarEvaluator *eval,
                           bool preferred_only);
    virtual ~StandardScalarOpenList() override;

    virtual void insert(const Entry &entry) override;
    virtual Entry remove_min(std::vector<int> *key = 0) override;
    virtual bool empty() const override;
    virtual void clear() override;

    virtual void evaluate(int g, bool preferred) override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override;

    static OpenList<Entry> *_parse(OptionParser &parser);
};

#include "standard_scalar_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
