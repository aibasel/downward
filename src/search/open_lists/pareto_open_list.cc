#include "pareto_open_list.h"

#include "../evaluator.h"
#include "../open_list.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/hash.h"
#include "../utils/memory.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <cassert>
#include <deque>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

namespace pareto_open_list {
template<class Entry>
class ParetoOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;

    using Bucket = deque<Entry>;
    using KeyType = vector<int>;
    using BucketMap = utils::HashMap<KeyType, Bucket>;
    using KeySet = set<KeyType>;

    BucketMap buckets;
    KeySet nondominated;
    bool state_uniform_selection;
    vector<shared_ptr<Evaluator>> evaluators;

    bool dominates(const KeyType &v1, const KeyType &v2) const;
    bool is_nondominated(
        const KeyType &vec, KeySet &domination_candidates) const;
    void remove_key(const KeyType &key);

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    explicit ParetoOpenList(const Options &opts);
    virtual ~ParetoOpenList() override = default;

    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;

    static OpenList<Entry> *_parse(OptionParser &p);
};

template<class Entry>
ParetoOpenList<Entry>::ParetoOpenList(const Options &opts)
    : OpenList<Entry>(opts.get<bool>("pref_only")),
      rng(utils::parse_rng_from_options(opts)),
      state_uniform_selection(opts.get<bool>("state_uniform_selection")),
      evaluators(opts.get_list<shared_ptr<Evaluator>>("evals")) {
}

template<class Entry>
bool ParetoOpenList<Entry>::dominates(
    const KeyType &v1, const KeyType &v2) const {
    assert(v1.size() == v2.size());
    bool are_different = false;
    for (size_t i = 0; i < v1.size(); ++i) {
        if (v1[i] > v2[i])
            return false;
        else if (v1[i] < v2[i])
            are_different = true;
    }
    return are_different;
}

template<class Entry>
bool ParetoOpenList<Entry>::is_nondominated(
    const KeyType &vec, KeySet &domination_candidates) const {
    for (const KeyType &candidate : domination_candidates)
        if (dominates(candidate, vec))
            return false;
    return true;
}

template<class Entry>
void ParetoOpenList<Entry>::remove_key(const KeyType &key) {
    /*
      We must copy the key because it is likely to live inside the
      data structures from which we remove it here and hence becomes
      invalid at that point.
    */
    vector<int> copied_key(key);
    nondominated.erase(copied_key);
    buckets.erase(copied_key);
    KeySet candidates;
    for (const auto &bucket_pair : buckets) {
        const KeyType &bucket_key = bucket_pair.first;
        /*
          If the estimate vector of the bucket is not already in the
          set of nondominated estimate vectors and the vector was
          previously dominated by key and the estimate vector is not
          dominated by any vector from the set of nondominated
          vectors, we add it to the candidates.
        */
        if (!nondominated.count(bucket_key) &&
            dominates(copied_key, bucket_key) &&
            is_nondominated(bucket_key, nondominated))
            candidates.insert(bucket_key);
    }
    for (const KeyType &candidate : candidates)
        if (is_nondominated(candidate, candidates))
            nondominated.insert(candidate);
}

template<class Entry>
void ParetoOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    vector<int> key;
    key.reserve(evaluators.size());
    for (const shared_ptr<Evaluator> &evaluator : evaluators)
        key.push_back(eval_context.get_evaluator_value_or_infinity(evaluator.get()));

    Bucket &bucket = buckets[key];
    bool newkey = bucket.empty();
    bucket.push_back(entry);

    if (newkey && is_nondominated(key, nondominated)) {
        /*
          Delete previously nondominated keys that are now dominated
          by key.

          Note: this requires that nondominated is a "normal"
          set (no hash set) because then iterators are not
          invalidated by erase(it).
        */
        auto it = nondominated.begin();
        while (it != nondominated.end()) {
            if (dominates(key, *it)) {
                auto tmp_it = it;
                ++it;
                nondominated.erase(tmp_it);
            } else {
                ++it;
            }
        }
        // Insert new key.
        nondominated.insert(key);
    }
}

template<class Entry>
Entry ParetoOpenList<Entry>::remove_min() {
    typename KeySet::iterator selected = nondominated.begin();
    int seen = 0;
    for (typename KeySet::iterator it = nondominated.begin();
         it != nondominated.end(); ++it) {
        int numerator;
        if (state_uniform_selection)
            numerator = it->size();
        else
            numerator = 1;
        seen += numerator;
        if ((*rng)(seen) < numerator)
            selected = it;
    }
    Bucket &bucket = buckets[*selected];
    Entry result = bucket.front();
    bucket.pop_front();
    if (bucket.empty())
        remove_key(*selected);
    return result;
}

template<class Entry>
bool ParetoOpenList<Entry>::empty() const {
    return nondominated.empty();
}

template<class Entry>
void ParetoOpenList<Entry>::clear() {
    buckets.clear();
    nondominated.clear();
}

template<class Entry>
void ParetoOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    for (const shared_ptr<Evaluator> &evaluator : evaluators)
        evaluator->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool ParetoOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    // TODO: Document this behaviour.
    // If one safe heuristic detects a dead end, return true.
    if (is_reliable_dead_end(eval_context))
        return true;
    // Otherwise, return true if all heuristics agree this is a dead-end.
    for (const shared_ptr<Evaluator> &evaluator : evaluators)
        if (!eval_context.is_evaluator_value_infinite(evaluator.get()))
            return false;
    return true;
}

template<class Entry>
bool ParetoOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    for (const shared_ptr<Evaluator> &evaluator : evaluators)
        if (eval_context.is_evaluator_value_infinite(evaluator.get()) &&
            evaluator->dead_ends_are_reliable())
            return true;
    return false;
}

ParetoOpenListFactory::ParetoOpenListFactory(
    const Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
ParetoOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<ParetoOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
ParetoOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<ParetoOpenList<EdgeOpenListEntry>>(options);
}

static shared_ptr<OpenListFactory> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Pareto open list",
        "Selects one of the Pareto-optimal (regarding the sub-evaluators) "
        "entries for removal.");

    parser.add_list_option<shared_ptr<Evaluator>>("evals", "evaluators");
    parser.add_option<bool>(
        "pref_only",
        "insert only nodes generated by preferred operators", "false");
    parser.add_option<bool>(
        "state_uniform_selection",
        "When removing an entry, we select a non-dominated bucket "
        "and return its oldest entry. If this option is false, we select "
        "uniformly from the non-dominated buckets; if the option is true, "
        "we weight the buckets with the number of entries.",
        "false");

    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<ParetoOpenListFactory>(opts);
}

static Plugin<OpenListFactory> _plugin("pareto", _parse);
}
