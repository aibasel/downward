#ifndef OPEN_LISTS_TYPED_OPEN_LIST_H
#define OPEN_LISTS_TYPED_OPEN_LIST_H

#include "open_list.h"
#include "pareto_open_list.h"
#include "../evaluator.h"
#include "../plugin.h"


#include <cstddef>
#include <unordered_map>
#include <vector>

class Options;
class OptionParser;


template<class Entry>
class TypedOpenList : public OpenList<Entry> {
    typedef std::vector<Entry> Bucket;
    std::vector<ScalarEvaluator *> evaluators;

    std::vector<std::pair<std::size_t, Bucket> > bucket_list;

    typedef typename std::unordered_map<std::size_t, int> BucketMap;
    BucketMap key_to_bucket_index;

    int size;
    bool dead_end;
    bool dead_end_reliable;

protected:
    Evaluator *get_evaluator() {return this; }

public:
    TypedOpenList(const Options &opts);
    TypedOpenList(const std::vector<OpenList<Entry> *> &sublists);
    virtual ~TypedOpenList();

    // OpenList interface
    virtual int insert(const Entry &entry);
    virtual Entry remove_min(std::vector<int> *key = 0);
    virtual bool empty() const;
    virtual void clear();

    // Evaluator interface
    virtual void evaluate(int g, bool preferred);
    virtual bool is_dead_end() const;
    virtual bool dead_end_is_reliable() const;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset);

    static OpenList<Entry> *_parse(OptionParser &parser);
};

#include "typed_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.
#endif
