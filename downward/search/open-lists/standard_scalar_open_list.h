#ifndef OPEN_LISTS_STANDARD_SCALAR_OPEN_LIST_H
#define OPEN_LISTS_STANDARD_SCALAR_OPEN_LIST_H

#include "open_list.h"

#include <deque>
#include <map>
#include <vector>
#include <utility>

class ScalarEvaluator;

template<class Entry>
class StandardScalarOpenList : public OpenList<Entry> {
    
    typedef std::deque<Entry> Bucket;

    std::map<int, Bucket> buckets;
    int size;
    mutable int lowest_bucket;
    
    ScalarEvaluator *evaluator;
    int last_evaluated_value;
    int last_preferred;
    bool dead_end;
    bool dead_end_reliable;

protected:
    ScalarEvaluator* get_evaluator() { return evaluator; }

public:
    StandardScalarOpenList(ScalarEvaluator *eval, 
                           bool preferred_only=false);
    ~StandardScalarOpenList();
    
    int insert(const Entry &entry);
    Entry remove_min();
    bool empty() const;
    void clear();
    
    void evaluate(int g, bool preferred);
    bool is_dead_end() const;
    bool dead_end_is_reliable() const;
};

#include "standard_scalar_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
