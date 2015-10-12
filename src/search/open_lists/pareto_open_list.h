#ifndef OPEN_LISTS_PARETO_OPEN_LIST_H
#define OPEN_LISTS_PARETO_OPEN_LIST_H

#include "open_list.h"

#include "../utilities_hash.h"

#include <deque>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

class OptionParser;
class Options;
class ScalarEvaluator;

template<class Entry>
class ParetoOpenList : public OpenList<Entry> {
    typedef std::deque<Entry> Bucket;
    typedef std::vector<int> KeyType;
    typedef std::unordered_map<KeyType, Bucket> BucketMap;
    typedef std::set<KeyType> KeySet;

    BucketMap buckets;
    KeySet nondominated;
    bool state_uniform_selection;
    std::vector<ScalarEvaluator *> evaluators;

    bool dominates(const KeyType &v1, const KeyType &v2) const;
    bool is_nondominated(
        const KeyType &vec, KeySet &domination_candidates) const;
    void remove_key(const KeyType &key);

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    explicit ParetoOpenList(const Options &opts);
    ParetoOpenList(const std::vector<ScalarEvaluator *> &evals,
                   bool preferred_only, bool state_uniform_selection);
    virtual ~ParetoOpenList() override = default;

    virtual Entry remove_min(std::vector<int> *key = 0) override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;

    static OpenList<Entry> *_parse(OptionParser &p);
};

#include "pareto_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif
