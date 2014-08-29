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

    std::vector<std::pair<std::vector<int>,Bucket> > bucket_list;

    typedef typename __gnu_cxx::hash_map<std::vector<int>, int, __gnu_cxx::hash<const std::vector<int> > > BucketMap;
    BucketMap open_list;

    int size;
    bool dead_end;
    bool dead_end_reliable;

    // removes the element at pos
    // returns the elemnet which had been moved
    template<typename T>
    T& fast_remove_from_vector(std::vector<T> &vec, size_t pos) {
        T &last = vec.back();
        if(vec.size() > 1 && (vec.size() - 1 != pos)) {
            //swap: move the back to the position to be deleted and then pop_back
            vec[pos] = last;
        }
        vec.pop_back();
        return last;
    }

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
