// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_TYPED_OPEN_LIST_H

#include "../globals.h"
#include "../option_parser.h"
#include "../rng.h"
#include "../utilities.h"

using namespace std;

template<class Entry>
TypedOpenList<Entry>::TypedOpenList(const Options &opts)
    : evaluators(opts.get_list<ScalarEvaluator *>("sublists")),
      size(0),
      dead_end(false),
      dead_end_reliable(false) {
}

template<class Entry>
TypedOpenList<Entry>::~TypedOpenList() {
}

template<class Entry>
int TypedOpenList<Entry>::insert(const Entry &entry) {
    vector<int> key(evaluators.size());
    for (size_t i = 0; i < evaluators.size(); ++i) {
        key[i] = evaluators[i]->get_value();
    }
    // The hash function is located in pareto_open_list.h
    size_t hash = __gnu_cxx::hash< const vector<int> >() (key);

    typename KeyToBucketIndex::iterator it = key_to_bucket_index.find(hash);
    if (it == key_to_bucket_index.end()) {
        keys_and_buckets.push_back(make_pair(hash, Bucket()));
        keys_and_buckets.back().second.push_back(entry); // TODO: c++11 list init
        key_to_bucket_index[hash] = keys_and_buckets.size() - 1;
    } else {
        size_t bucket_index = it->second;
        assert(bucket_index < keys_and_buckets.size());
        keys_and_buckets[bucket_index].second.push_back(entry);
    }

    ++size;
    return 1;
}

template<class Entry>
Entry TypedOpenList<Entry>::remove_min(vector<int> *key) {
    assert(size > 0);

    if (key) {
        cerr << "not implemented -- see msg639 in the tracker" << endl;
        exit_with(EXIT_UNSUPPORTED);
    }

    int bucket_id = g_rng.next(keys_and_buckets.size());
    pair<size_t, Bucket> &key_and_bucket = keys_and_buckets[bucket_id];
    size_t hash = key_and_bucket.first;
    Bucket &bucket = key_and_bucket.second;

    int pos = g_rng.next(bucket.size());
    Entry result = bucket[pos];

    swap_and_pop_from_vector(bucket, pos);
    if (bucket.empty()) {
        // Swap the empty bucket with the last bucket, then delete it.
        size_t moved_bucket_hash = keys_and_buckets.back().first;
        key_to_bucket_index[moved_bucket_hash] = bucket_id;
        assert(bucket_id < keys_and_buckets.size());
        swap(keys_and_buckets[bucket_id], keys_and_buckets.back());
        keys_and_buckets.pop_back();
        key_to_bucket_index.erase(hash);
    }
    --size;
    return result;
}

template<class Entry>
bool TypedOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void TypedOpenList<Entry>::clear() {
    keys_and_buckets.clear();
    key_to_bucket_index.clear();
    size = 0;
}

template<class Entry>
void TypedOpenList<Entry>::evaluate(int g, bool preferred) {
    /*
      Treat as a dead end if
      1. at least one heuristic reliably recognizes it as a dead end, or
      2. all heuristics unreliably recognize it as a dead end
      In case 1., the dead end is reliable; in case 2. it is not.
    */

    dead_end = true;
    dead_end_reliable = false;
    for (size_t i = 0; i < evaluators.size(); i++) {
        evaluators[i]->evaluate(g, preferred);
        if (evaluators[i]->is_dead_end()) {
            if (evaluators[i]->dead_end_is_reliable()) {
                dead_end = true; // Might have been set to false.
                dead_end_reliable = true;
                break;
            }
        } else {
            dead_end = false;
        }
    }
}

template<class Entry>
bool TypedOpenList<Entry>::is_dead_end() const {
    return dead_end;
}

template<class Entry>
bool TypedOpenList<Entry>::dead_end_is_reliable() const {
    return dead_end_reliable;
}

template<class Entry>
void TypedOpenList<Entry>::get_involved_heuristics(set<Heuristic *> &hset) {
    for (size_t i = 0; i < evaluators.size(); i++)
        evaluators[i]->get_involved_heuristics(hset);
}

template<class Entry>
OpenList<Entry> *TypedOpenList<Entry>::_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Type-based open list",
        "Type-based open list that uses multiple evaluators to put nodes "
        "into buckets. When retrieving a node, a bucket is chosen "
        "uniformly at random and one of the contained nodes is selected "
        "randomly.");
    parser.add_list_option<ScalarEvaluator *>("sublists", "The evaluators to group the nodes by.");

    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    opts.verify_list_non_empty<ScalarEvaluator *>("sublists");
    if (parser.dry_run())
        return 0;
    else
        return new TypedOpenList<Entry>(opts);
}

#endif
