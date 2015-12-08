#include "type_based_open_list.h"

#include "open_list.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../rng.h"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace std;


template<class Entry>
class TypeBasedOpenList : public OpenList<Entry> {
    vector<ScalarEvaluator *> evaluators;

    using Key = vector<int>;
    using Bucket = vector<Entry>;
    vector<pair<Key, Bucket>> keys_and_buckets;
    unordered_map<Key, int> key_to_bucket_index;

protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

public:
    explicit TypeBasedOpenList(const Options &opts);
    virtual ~TypeBasedOpenList() override = default;

    virtual Entry remove_min(vector<int> *key = nullptr) override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual bool is_dead_end(EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
    virtual void get_involved_heuristics(set<Heuristic *> &hset) override;
};

template<class Entry>
void TypeBasedOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    vector<int> key;
    key.reserve(evaluators.size());
    for (ScalarEvaluator *evaluator : evaluators) {
        key.push_back(
            eval_context.get_heuristic_value_or_infinity(evaluator));
    }

    auto it = key_to_bucket_index.find(key);
    if (it == key_to_bucket_index.end()) {
        key_to_bucket_index[key] = keys_and_buckets.size();
        keys_and_buckets.push_back({move(key), {entry}});
    } else {
        size_t bucket_index = it->second;
        assert(in_bounds(bucket_index, keys_and_buckets));
        keys_and_buckets[bucket_index].second.push_back(entry);
    }
}

template<class Entry>
TypeBasedOpenList<Entry>::TypeBasedOpenList(const Options &opts)
    : evaluators(opts.get_list<ScalarEvaluator *>("evaluators")) {
}

template<class Entry>
Entry TypeBasedOpenList<Entry>::remove_min(vector<int> *key) {
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

template<class Entry>
bool TypeBasedOpenList<Entry>::empty() const {
    return keys_and_buckets.empty();
}

template<class Entry>
void TypeBasedOpenList<Entry>::clear() {
    keys_and_buckets.clear();
    key_to_bucket_index.clear();
}

template<class Entry>
bool TypeBasedOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
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

template<class Entry>
bool TypeBasedOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    for (ScalarEvaluator *evaluator : evaluators) {
        if (evaluator->dead_ends_are_reliable() &&
            eval_context.is_heuristic_infinite(evaluator))
            return true;
    }
    return false;
}

template<class Entry>
void TypeBasedOpenList<Entry>::get_involved_heuristics(
    set<Heuristic *> &hset) {
    for (ScalarEvaluator *evaluator : evaluators) {
        evaluator->get_involved_heuristics(hset);
    }
}

TypeBasedOpenListFactory::TypeBasedOpenListFactory(
    const Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
TypeBasedOpenListFactory::create_state_open_list() {
    return make_unique_ptr<TypeBasedOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
TypeBasedOpenListFactory::create_edge_open_list() {
    return make_unique_ptr<TypeBasedOpenList<EdgeOpenListEntry>>(options);
}

static shared_ptr<OpenListFactory> _parse(OptionParser &parser) {
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
    opts.verify_list_non_empty<ScalarEvaluator *>("evaluators");
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<TypeBasedOpenListFactory>(opts);
}

static PluginShared<OpenListFactory> _plugin("type_based", _parse);
