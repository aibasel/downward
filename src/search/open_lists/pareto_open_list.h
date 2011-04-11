#ifndef OPEN_LISTS_PARETO_OPEN_LIST_H
#define OPEN_LISTS_PARETO_OPEN_LIST_H

#include "open_list.h"
#include "../evaluator.h"
#include "../option_parser.h"

#include <deque>
#include <ext/hash_map>
#include <set>
#include <vector>
#include <utility>

class ScalarEvaluator;

namespace __gnu_cxx {
template<>
struct hash<const std::vector<int> > {
    // hash function adapted from Python's hash function for tuples.
    size_t operator()(const std::vector<int> &vec) const {
        size_t hash_value = 0x345678;
        size_t mult = 1000003;
        for (int i = vec.size() - 1; i >= 0; i--) {
            hash_value = (hash_value ^ vec[i]) * mult;
            mult += 82520 + i + i;
        }
        hash_value += 97531;
        return hash_value;
    }
};
}

template<class Entry>
class ParetoOpenList : public OpenList<Entry> {
    typedef std::deque<Entry> Bucket;
    typedef std::vector<int> KeyType;
    //typedef std::map<const KeyType, Bucket> BucketMap;
    typedef typename __gnu_cxx::hash_map<const KeyType, Bucket,
                                         __gnu_cxx::hash<const std::vector<int> > > BucketMap;
    typedef std::set<KeyType> KeySet; // no hash_set (see insert method)

    BucketMap buckets;
    KeySet nondominated;
    bool state_uniform_selection;
    std::vector<ScalarEvaluator *> evaluators;
    std::vector<int> last_evaluated_value;

    bool dominates(const KeyType &v1, const KeyType &v2);
    bool is_nondominated(const KeyType &vec,
                         KeySet &domination_candidates);
    void remove_key(const KeyType key);
    bool last_preferred;

    bool dead_end;
    bool dead_end_reliable;
protected:
    Evaluator *get_evaluator() {return this; }

public:
    ParetoOpenList(const std::vector<ScalarEvaluator *> &evals,
                   bool preferred_only, bool state_uniform_selection_);
    ParetoOpenList(const Options &opts);
    ~ParetoOpenList();

    // open list interface
    int insert(const Entry &entry);
    Entry remove_min(std::vector<int> *key = 0);
    bool empty() const {return nondominated.empty(); }
    void clear();

    // tuple evaluator interface
    void evaluate(int g, bool preferred);
    bool is_dead_end() const;
    bool dead_end_is_reliable() const;
    void get_involved_heuristics(std::set<Heuristic *> &hset);

    static OpenList<Entry> *_parse(OptionParser &p);
};

#include "pareto_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
