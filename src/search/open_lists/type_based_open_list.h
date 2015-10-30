#ifndef OPEN_LISTS_TYPE_BASED_OPEN_LIST_H
#define OPEN_LISTS_TYPE_BASED_OPEN_LIST_H

#include "open_list.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../rng.h"
#include "../utilities.h"
#include "../utilities_hash.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

/*
  Type-based open list based on Xie et al. (AAAI 2014), see below.

  The original implementation uses a std::map for storing and looking
  up buckets. Our implementation stores the Buckets in a std::vector
  and uses a std::unordered_map for looking up indexes in the vector.

  In the table below we list the amortized worst-case time complexities
  for the original implementation and the version below.

    n = number of entries
    m = number of buckets

                            Original    Code below
    Insert entry            O(log(m))   O(1)
    Remove entry            O(m)        O(1)        # both use swap+pop
*/

template<class Entry>
class TypeBasedOpenList : public OpenList<Entry> {
    std::vector<ScalarEvaluator *> evaluators;

    using Key = std::vector<int>;
    using Bucket = std::vector<Entry>;
    std::vector<std::pair<Key, Bucket>> keys_and_buckets;
    std::unordered_map<Key, int> key_to_bucket_index;

protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override {
        std::vector<int> key;
        key.reserve(evaluators.size());
        for (ScalarEvaluator *evaluator : evaluators) {
            key.push_back(
                eval_context.get_heuristic_value_or_infinity(evaluator));
        }

        auto it = key_to_bucket_index.find(key);
        if (it == key_to_bucket_index.end()) {
            key_to_bucket_index[key] = keys_and_buckets.size();
            keys_and_buckets.push_back({move(key), {entry}
                                       });
        } else {
            size_t bucket_index = it->second;
            assert(in_bounds(bucket_index, keys_and_buckets));
            keys_and_buckets[bucket_index].second.push_back(entry);
        }
    }

public:
    explicit TypeBasedOpenList(const Options &opts)
        : evaluators(opts.get_list<ScalarEvaluator *>("evaluators")) {
    }

    virtual ~TypeBasedOpenList() override = default;

    virtual Entry remove_min(std::vector<int> *key = 0) override {
        size_t bucket_id = g_rng(keys_and_buckets.size());
        auto &key_and_bucket = keys_and_buckets[bucket_id];
        const Key &min_key = key_and_bucket.first;
        Bucket &bucket = key_and_bucket.second;

        if (key) {
            assert(key->empty());
            *key = min_key;
        }

        int pos = g_rng(bucket.size());
        Entry result = swap_and_pop_from_vector(bucket, pos);

        if (bucket.empty()) {
            // Swap the empty bucket with the last bucket, then delete it.
            key_to_bucket_index[keys_and_buckets.back().first] = bucket_id;
            key_to_bucket_index.erase(min_key);
            swap_and_pop_from_vector(keys_and_buckets, bucket_id);
        }
        return result;
    }

    virtual bool empty() const override {
        return keys_and_buckets.empty();
    }

    virtual void clear() override {
        keys_and_buckets.clear();
        key_to_bucket_index.clear();
    }

    virtual bool is_dead_end(EvaluationContext &eval_context) const override {
        // If one evaluator is sure we have a dead end, return true.
        if (is_reliable_dead_end(eval_context))
            return true;
        // Otherwise, return true if all evaluators agree this is a dead-end.
        for (ScalarEvaluator *evaluator : evaluators) {
            if (!eval_context.is_heuristic_infinite(evaluator))
                return false;
        }
        return true;
    }

    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override {
        for (ScalarEvaluator *evaluator : evaluators) {
            if (evaluator->dead_ends_are_reliable() &&
                eval_context.is_heuristic_infinite(evaluator))
                return true;
        }
        return false;
    }

    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override {
        for (ScalarEvaluator *evaluator : evaluators) {
            evaluator->get_involved_heuristics(hset);
        }
    }

    static OpenList<Entry> *_parse(OptionParser &parser) {
        parser.document_synopsis(
            "Type-based open list",
            "Uses multiple evaluators to assign entries to buckets. "
            "All entries in a bucket have the same evaluator values. "
            "When retrieving an entry, a bucket is chosen uniformly at "
            "random and one of the contained entries is selected "
            "uniformly randomly. "
            "The algorithm is based on\n\n"
            " * Fan Xie, Martin Mueller, Robert Holte, Tatsuya Imai.<<BR>>\n"
            " [Type-Based Exploration with Multiple Search Queues for "
            "Satisficing Planning "
            "http://www.aaai.org/ocs/index.php/AAAI/AAAI14/paper/view/8472/8705]."
            "<<BR>>\n "
            "In //Proceedings of the Twenty-Eigth AAAI Conference "
            "Conference on Artificial Intelligence (AAAI "
            "2014)//, pp. 2395-2401. AAAI Press 2014.\n\n\n");
        parser.add_list_option<ScalarEvaluator *>(
            "evaluators",
            "Evaluators used to determine the bucket for each entry.");

        Options opts = parser.parse();
        if (parser.help_mode())
            return nullptr;

        opts.verify_list_non_empty<ScalarEvaluator *>("evaluators");
        if (parser.dry_run())
            return nullptr;
        else
            return new TypeBasedOpenList<Entry>(opts);
    }
};

#endif
