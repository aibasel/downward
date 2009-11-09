#ifndef PARETO_OPENLIST_H
#define PARETO_OPENLIST_H

#include "open_list.h"
#include "../evaluator.h"

#include <deque>
#include <ext/hash_set>
#include <map>
#include <vector>
#include <utility>

class ScalarEvaluator;

namespace __gnu_cxx {
    template<> struct hash<const std::vector<int> > {
    // hash function adapted from Python's hash function for tuples.
        size_t operator()(const std::vector<int> &vec) const {
            size_t hash_value = 0x345678;
            size_t mult = 1000003;
            for(int i = vec.size() - 1; i >= 0; i--) {
                hash_value = (hash_value ^ vec[i]) * mult;
                mult += 82520 + i + i;
            }
            hash_value += 97531;
            return hash_value;
        }
    };
}

template<class Entry>
class ParetoOpenList : public OpenList<Entry>, public Evaluator {
    
    typedef std::deque<Entry> Bucket;
    typedef const std::vector<int> KeyType;
    typedef std::map<KeyType, Bucket> BucketMap;
    typedef typename __gnu_cxx::hash_set<KeyType, __gnu_cxx::hash<const std::vector<int> > > KeySet;

    BucketMap buckets;
    KeySet nondominated;
    std::vector<ScalarEvaluator *> evaluators;
    
    bool dominates(const KeyType &v1, const KeyType &v2);
    bool is_nondominated(const KeyType &vec,
            KeySet &domination_candidates);
    void remove_key(const KeyType key);
	bool last_preferred;
    
    bool dead_end;
    bool dead_end_reliable;
    
protected:
    Evaluator* get_evaluator() { return this; }

public:
    ParetoOpenList(const std::vector<ScalarEvaluator *> &evals, 
                   bool preferred_only=false);
    ~ParetoOpenList();

    // open list interface
    int insert(const Entry &entry);
    Entry remove_min();
    bool empty() const { return nondominated.empty(); }

    // tuple evaluator interface
    void evaluate(int g, bool preferred);
    bool is_dead_end() const;
    bool dead_end_is_reliable() const;
};

#include "pareto_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
