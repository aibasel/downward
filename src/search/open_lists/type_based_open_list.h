#ifndef OPEN_LISTS_TYPE_BASED_OPEN_LIST_H
#define OPEN_LISTS_TYPE_BASED_OPEN_LIST_H

#include "open_list.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../rng.h"
#include "../utilities.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

/*
  Type-based open list that uses multiple evaluators to put nodes into buckets.
  A bucket contains all entries with the same evaluator results.
  When retrieving a node, a bucket is chosen uniformly at random
  and one of the contained nodes is selected uniformly randomly.
  Based on:
    Fan Xie, Martin Mueller, Robert C. Holte, and Tatsuya Imai.
    Type-Based Exploration with Multiple Search Queues for Satisficing Planning.
    In Proceedings of the Twenty-Eighth AAAI Conference on Artificial
    Intelligence (AAAI 2014). AAAI Press, 2014.
*/

template<class Entry>
class TypeBasedOpenList : public OpenList<Entry> {
    std::vector<ScalarEvaluator *> evaluators;

    typedef std::vector<int> Key;
    typedef std::vector<Entry> Bucket;
    std::vector<std::pair<Key, Bucket> > keys_and_buckets;

    typedef typename std::unordered_map<Key, int, hash_int_vector> KeyToBucketIndex;
    KeyToBucketIndex key_to_bucket_index;

    int size;
    bool dead_end;
    bool dead_end_reliable;

protected:
    virtual Evaluator *get_evaluator() override {
        return this;
    }

public:
    explicit TypeBasedOpenList(const Options &opts)
        : evaluators(opts.get_list<ScalarEvaluator *>("evaluators")),
          size(0),
          dead_end(false),
          dead_end_reliable(false) {
    }

    virtual ~TypeBasedOpenList() override = default;

    virtual int insert(const Entry &entry) override {
        std::vector<int> key;
        key.reserve(evaluators.size());
        for (ScalarEvaluator *evaluator : evaluators) {
            key.push_back(evaluator->get_value());
        }

        typename KeyToBucketIndex::iterator it = key_to_bucket_index.find(key);
        if (it == key_to_bucket_index.end()) {
            keys_and_buckets.push_back(make_pair(key, Bucket {entry}
                                                 ));
            key_to_bucket_index[key] = keys_and_buckets.size() - 1;
        } else {
            size_t bucket_index = it->second;
            assert(bucket_index < keys_and_buckets.size());
            keys_and_buckets[bucket_index].second.push_back(entry);
        }

        ++size;
        return 1;
    }

    virtual Entry remove_min(std::vector<int> *key = 0) override {
        assert(size > 0);
        size_t bucket_id = g_rng(keys_and_buckets.size());
        std::pair<Key, Bucket> &key_and_bucket = keys_and_buckets[bucket_id];
        Bucket &bucket = key_and_bucket.second;

        if (key) {
            assert(key->empty());
            *key = key_and_bucket.first;
        }

        int pos = g_rng(bucket.size());
        Entry result = bucket[pos];

        swap_and_pop_from_vector(bucket, pos);
        if (bucket.empty()) {
            // Swap the empty bucket with the last bucket, then delete it.
            Key key_copy = key_and_bucket.first;
            Key moved_bucket_key = keys_and_buckets.back().first;
            key_to_bucket_index[moved_bucket_key] = bucket_id;
            assert(bucket_id < keys_and_buckets.size());
            swap(keys_and_buckets[bucket_id], keys_and_buckets.back());
            keys_and_buckets.pop_back();
            key_to_bucket_index.erase(key_copy);
        }
        --size;
        return result;
    }

    virtual bool empty() const override {
        return size == 0;
    }

    virtual void clear() override {
        keys_and_buckets.clear();
        key_to_bucket_index.clear();
        size = 0;
    }

    virtual void evaluate(int g, bool preferred) override {
        // The code is taken from AlternationOpenList
        // TODO: see issue494, common implementation of evaluate for multi evaluator open lists

        dead_end = true;
        dead_end_reliable = false;
        for (ScalarEvaluator *evaluator : evaluators) {
            evaluator->evaluate(g, preferred);
            if (evaluator->is_dead_end()) {
                if (evaluator->dead_end_is_reliable()) {
                    dead_end = true; // Might have been set to false.
                    dead_end_reliable = true;
                    break;
                }
            } else {
                dead_end = false;
            }
        }
    }

    virtual bool is_dead_end() const override {
        return dead_end;
    }

    virtual bool dead_end_is_reliable() const override {
        return dead_end_reliable;
    }

    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override {
        for (ScalarEvaluator *evaluator : evaluators) {
            evaluator->get_involved_heuristics(hset);
        }
    }

    static OpenList<Entry> *_parse(OptionParser &parser) {
        parser.document_synopsis(
            "Type-based open list",
            "Type-based open list that uses multiple evaluators to put nodes "
            "into buckets. A bucket contains all entries with the same "
            "evaluator results. When retrieving a node, a bucket is chosen "
            "uniformly at random and one of the contained nodes is selected "
            "uniformly randomly.");
        parser.add_list_option<ScalarEvaluator *>(
                    "evaluators",
                    "Evaluators used to determine the bucket for each entry.");

        Options opts = parser.parse();
        if (parser.help_mode())
            return 0;

        opts.verify_list_non_empty<ScalarEvaluator *>("evaluators");
        if (parser.dry_run())
            return 0;
        else
            return new TypeBasedOpenList<Entry>(opts);
    }
};

#endif
