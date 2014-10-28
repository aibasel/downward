#ifndef OPEN_LISTS_TYPE_BASED_OPEN_LIST_H
#define OPEN_LISTS_TYPE_BASED_OPEN_LIST_H

#include "open_list.h"

#include "../plugin.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

class Options;
class OptionParser;
class ScalarEvaluator;


template<class Entry>
class TypeBasedOpenList : public OpenList<Entry> {
    typedef std::vector<int> Key;
    typedef std::vector<Entry> Bucket;
    std::vector<ScalarEvaluator *> evaluators;

    std::vector<std::pair<Key, Bucket> > keys_and_buckets;

    typedef typename std::unordered_map<Key, int, hash_int_vector> KeyToBucketIndex;
    KeyToBucketIndex key_to_bucket_index;

    int size;
    bool dead_end;
    bool dead_end_reliable;

protected:
    Evaluator *get_evaluator() {return this; }

public:
    explicit TypeBasedOpenList(const Options &opts);
    virtual ~TypeBasedOpenList();

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
