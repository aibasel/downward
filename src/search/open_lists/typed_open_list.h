#ifndef OPEN_LISTS_TYPED_OPEN_LIST_H
#define OPEN_LISTS_TYPED_OPEN_LIST_H

#include "open_list.h"
#include "../evaluator.h"
#include "../plugin.h"
#include "pareto_open_list.h"

#include <vector>

class Options;
class OptionParser;


template<class Entry>
class TypedOpenList : public OpenList<Entry> {
    typedef std::vector<Entry> Bucket;
    std::vector<ScalarEvaluator *> evaluators;
    //TODO: use ordered std::map?
    //TODO: switch to c++11 std::unordered_map
    typedef typename __gnu_cxx::hash_map<std::vector<int>, Bucket, __gnu_cxx::hash<const std::vector<int> > > BucketMap;
    BucketMap open_list;


    int size;
    bool dead_end;
    bool dead_end_reliable;

protected:
    Evaluator *get_evaluator() {return this; }

public:
    TypedOpenList(const Options &opts);
    TypedOpenList(const std::vector<OpenList<Entry> *> &sublists);
    ~TypedOpenList();

    // OpenList interface
    int insert(const Entry &entry);
    Entry remove_min(std::vector<int> *key = 0);
    bool empty() const;
    void clear();

    // Evaluator interface
    void evaluate(int g, bool preferred);
    bool is_dead_end() const;
    bool dead_end_is_reliable() const;
    void get_involved_heuristics(std::set<Heuristic *> &hset);


    static OpenList<Entry> *_parse(OptionParser &parser);
};

#include "typed_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
