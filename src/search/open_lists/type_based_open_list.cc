#include "type_based_open_list.h"

#include "../evaluator.h"
#include "../open_list.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/collections.h"
#include "../utils/hash.h"
#include "../utils/markup.h"
#include "../utils/memory.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace std;

namespace type_based_open_list {
template<class Entry>
class TypeBasedOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;
    vector<shared_ptr<Evaluator>> evaluators;

    using Key = vector<int>;
    using Bucket = vector<Entry>;
    vector<pair<Key, Bucket>> keys_and_buckets;
    utils::HashMap<Key, int> key_to_bucket_index;

protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

public:
    explicit TypeBasedOpenList(const Options &opts);
    virtual ~TypeBasedOpenList() override = default;

    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual bool is_dead_end(EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;
};

template<class Entry>
void TypeBasedOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    vector<int> key;
    key.reserve(evaluators.size());
    for (const shared_ptr<Evaluator> &evaluator : evaluators) {
        key.push_back(
            eval_context.get_evaluator_value_or_infinity(evaluator.get()));
    }

    auto it = key_to_bucket_index.find(key);
    if (it == key_to_bucket_index.end()) {
        key_to_bucket_index[key] = keys_and_buckets.size();
        keys_and_buckets.push_back(make_pair(move(key), Bucket({entry})));
    } else {
        size_t bucket_index = it->second;
        assert(utils::in_bounds(bucket_index, keys_and_buckets));
        keys_and_buckets[bucket_index].second.push_back(entry);
    }
}

template<class Entry>
TypeBasedOpenList<Entry>::TypeBasedOpenList(const Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      evaluators(opts.get_list<shared_ptr<Evaluator>>("evaluators")) {
}

template<class Entry>
Entry TypeBasedOpenList<Entry>::remove_min() {
    size_t bucket_id = (*rng)(keys_and_buckets.size());
    auto &key_and_bucket = keys_and_buckets[bucket_id];
    const Key &min_key = key_and_bucket.first;
    Bucket &bucket = key_and_bucket.second;
    int pos = (*rng)(bucket.size());
    Entry result = utils::swap_and_pop_from_vector(bucket, pos);

    if (bucket.empty()) {
        // Swap the empty bucket with the last bucket, then delete it.
        key_to_bucket_index[keys_and_buckets.back().first] = bucket_id;
        key_to_bucket_index.erase(min_key);
        utils::swap_and_pop_from_vector(keys_and_buckets, bucket_id);
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
    for (const shared_ptr<Evaluator> &evaluator : evaluators) {
        if (!eval_context.is_evaluator_value_infinite(evaluator.get()))
            return false;
    }
    return true;
}

template<class Entry>
bool TypeBasedOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    for (const shared_ptr<Evaluator> &evaluator : evaluators) {
        if (evaluator->dead_ends_are_reliable() &&
            eval_context.is_evaluator_value_infinite(evaluator.get()))
            return true;
    }
    return false;
}

template<class Entry>
void TypeBasedOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    for (const shared_ptr<Evaluator> &evaluator : evaluators) {
        evaluator->get_path_dependent_evaluators(evals);
    }
}

TypeBasedOpenListFactory::TypeBasedOpenListFactory(
    const Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
TypeBasedOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<TypeBasedOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
TypeBasedOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<TypeBasedOpenList<EdgeOpenListEntry>>(options);
}

static shared_ptr<OpenListFactory> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Type-based open list",
        "Uses multiple evaluators to assign entries to buckets. "
        "All entries in a bucket have the same evaluator values. "
        "When retrieving an entry, a bucket is chosen uniformly at "
        "random and one of the contained entries is selected "
        "uniformly randomly. "
        "The algorithm is based on" + utils::format_paper_reference(
            {"Fan Xie", "Martin Mueller", "Robert Holte", "Tatsuya Imai"},
            "Type-Based Exploration with Multiple Search Queues for"
            " Satisficing Planning",
            "http://www.aaai.org/ocs/index.php/AAAI/AAAI14/paper/view/8472/8705",
            "Proceedings of the Twenty-Eigth AAAI Conference Conference"
            " on Artificial Intelligence (AAAI 2014)",
            "2395-2401",
            "AAAI Press 2014"));
    parser.add_list_option<shared_ptr<Evaluator>>(
        "evaluators",
        "Evaluators used to determine the bucket for each entry.");

    utils::add_rng_options(parser);

    Options opts = parser.parse();
    opts.verify_list_non_empty<shared_ptr<Evaluator>>("evaluators");
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<TypeBasedOpenListFactory>(opts);
}

static Plugin<OpenListFactory> _plugin("type_based", _parse);
}
