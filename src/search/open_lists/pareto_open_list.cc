// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_PARETO_OPEN_LIST_H

#include "../globals.h"
#include "../option_parser.h"
#include "../rng.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace std;

template<class Entry>
ParetoOpenList<Entry>::ParetoOpenList(const Options &opts)
    : OpenList<Entry>(opts.get<bool>("pref_only")),
      state_uniform_selection(opts.get<bool>("state_uniform_selection")),
      evaluators(opts.get_list<ScalarEvaluator *>("evals")) {
}

template<class Entry>
ParetoOpenList<Entry>::ParetoOpenList(
    const std::vector<ScalarEvaluator *> &evals,
    bool preferred_only,
    bool state_uniform_selection)
    : OpenList<Entry>(preferred_only),
      state_uniform_selection(state_uniform_selection),
      evaluators(evals) {
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
    for (ScalarEvaluator *evaluator : evaluators)
        key.push_back(eval_context.get_heuristic_value_or_infinity(evaluator));

    Bucket &bucket = buckets[key];
    bool newkey = bucket.empty();
    bucket.push_back(entry);

    if (newkey && is_nondominated(key, nondominated)) {
        // delete previously nondominated keys that are now
        // dominated by key
        // Note: this requirest that nondominated is a "normal"
        // set (no hash set) because then iterators are not
        // invalidated by erase(it).
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
        // insert new key
        nondominated.insert(key);
    }
}

template<class Entry>
Entry ParetoOpenList<Entry>::remove_min(vector<int> *key) {
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
        if (g_rng(seen) < numerator)
            selected = it;
    }
    if (key) {
        assert(key->empty());
        *key = *selected;
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
void ParetoOpenList<Entry>::get_involved_heuristics(
    std::set<Heuristic *> &hset) {
    for (ScalarEvaluator *evaluator : evaluators)
        evaluator->get_involved_heuristics(hset);
}

template<class Entry>
bool ParetoOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    // TODO: Document this behaviour.
    // If one safe heuristic detects a dead end, return true.
    if (is_reliable_dead_end(eval_context))
        return true;
    // Otherwise, return true if all heuristics agree this is a dead-end.
    for (ScalarEvaluator *evaluator : evaluators)
        if (!eval_context.is_heuristic_infinite(evaluator))
            return false;
    return true;
}

template<class Entry>
bool ParetoOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    for (ScalarEvaluator *evaluator : evaluators)
        if (eval_context.is_heuristic_infinite(evaluator) &&
            evaluator->dead_ends_are_reliable())
            return true;
    return false;
}

template<class Entry>
OpenList<Entry> *ParetoOpenList<Entry>::_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Pareto open list",
        "Selects one of the Pareto-optimal (regarding the sub-evaluators) "
        "entries for removal.");

    parser.add_list_option<ScalarEvaluator *>("evals", "scalar evaluators");
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

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return new ParetoOpenList<Entry>(opts);
}

#endif
