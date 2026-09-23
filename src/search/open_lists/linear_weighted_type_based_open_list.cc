#include "linear_weighted_type_based_open_list.h"

#include "../evaluator.h"
#include "../open_list.h"

#include "../utils/collections.h"
#include "../utils/hash.h"
#include "../utils/markup.h"
#include "../plugins/plugin.h"
#include "../utils/memory.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace std;

namespace linear_weighted_type_based_open_list {
template<class Entry>
class LinearWeightedTypeBasedOpenList : public OpenList<Entry> {
    utils::RandomNumberGenerator rng;
    vector<shared_ptr<Evaluator>> evaluators;

    using Key = vector<int>;
    using Bucket = vector<Entry>;
    unordered_map<int, vector<pair<Key, Bucket>>> first_to_keys_and_buckets;
    unordered_map<int, utils::HashMap<Key, int>> first_to_key_to_bucket_index;
    std::set<int> first_values;

    double alpha;
    double beta;
    bool ignore_size;
    bool ignore_weights;

protected:
    virtual void do_insertion(
        EvaluationContext &eval_context, const Entry &entry) override;

public:
    explicit LinearWeightedTypeBasedOpenList(const plugins::Options &opts);
    virtual ~LinearWeightedTypeBasedOpenList() override = default;

    virtual Entry remove_min() override;
    virtual bool empty() override;
    virtual void clear() override;
    virtual bool is_dead_end(EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;
};

template<class Entry>
void LinearWeightedTypeBasedOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    vector<int> key;
    key.reserve(evaluators.size() - 1);
    bool first = true;
    int key_first = -1;
    for (const shared_ptr<Evaluator> &evaluator : evaluators) {
        if (first) {
            key_first = eval_context.get_evaluator_value_or_infinity(evaluator.get());
            first = false;
        } else {
            key.push_back(
                eval_context.get_evaluator_value_or_infinity(evaluator.get()));
        }
    }

    auto first_it = first_to_key_to_bucket_index.find(key_first);
    if (first_it == first_to_key_to_bucket_index.end()) {
        first_values.insert(key_first);
        auto &keys_and_buckets = first_to_keys_and_buckets[key_first];
        first_to_key_to_bucket_index[key_first][key] = keys_and_buckets.size();
        keys_and_buckets.push_back(make_pair(move(key), Bucket({entry})));
    } else {
        auto &keys_and_buckets = first_to_keys_and_buckets[key_first];
        auto &key_to_bucket_index = first_to_key_to_bucket_index[key_first];
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
}

template<class Entry>
LinearWeightedTypeBasedOpenList<Entry>::LinearWeightedTypeBasedOpenList(const plugins::Options &opts)
    : rng(opts.get<int>("random_seed")),
      evaluators(opts.get_list<shared_ptr<Evaluator>>("evaluators")),
      alpha(opts.get<double>("alpha")),
      beta(opts.get<double>("beta")),
      ignore_size(opts.get<bool>("ignore_size")),
      ignore_weights(opts.get<bool>("ignore_weights")) {
}

template<class Entry>
Entry LinearWeightedTypeBasedOpenList<Entry>::remove_min() {
    int key_first = *first_values.begin();
    if (first_values.size() > 1) {
        double r = rng.random();
        double bias = static_cast<double>(*first_values.rbegin()) + beta;

        double numerator = 0.0;
        for (auto value : first_values) {
          double p = 1.0;

          if (!ignore_weights)
              p *= -1 * alpha * static_cast<double>(value) + bias;

          if (!ignore_size)
              p *= static_cast<double>(first_to_keys_and_buckets[value].size());

          numerator += p;
        }

        double p_sum = 0.0;
        for (auto value : first_values) {
            double p = 1.0 / numerator;

            if (!ignore_weights)
                p *= -1 * alpha * static_cast<double>(value) + bias;

            if (!ignore_size)
                p *= static_cast<double>(first_to_keys_and_buckets[value].size());

            p_sum += p;
            if (r <= p_sum) {
                key_first = value;
                break;
            }
        }
    }

    auto &keys_and_buckets = first_to_keys_and_buckets[key_first];
    auto &key_to_bucket_index = first_to_key_to_bucket_index[key_first];

    size_t bucket_id = rng.random(keys_and_buckets.size());
    auto &key_and_bucket = keys_and_buckets[bucket_id];
    const Key &min_key = key_and_bucket.first;
    Bucket &bucket = key_and_bucket.second;
    int pos = rng.random(bucket.size());
    Entry result = utils::swap_and_pop_from_vector(bucket, pos);

    if (bucket.empty()) {
        // Swap the empty bucket with the last bucket, then delete it.
        key_to_bucket_index[keys_and_buckets.back().first] = bucket_id;
        key_to_bucket_index.erase(min_key);
        utils::swap_and_pop_from_vector(keys_and_buckets, bucket_id);

        if (keys_and_buckets.empty()) {
            first_to_keys_and_buckets.erase(key_first);
            first_to_key_to_bucket_index.erase(key_first);
            first_values.erase(key_first);
        }
    }

    return result;
}

template<class Entry>
bool LinearWeightedTypeBasedOpenList<Entry>::empty() {
    return first_values.empty();
}

template<class Entry>
void LinearWeightedTypeBasedOpenList<Entry>::clear() {
    first_to_keys_and_buckets.clear();
    first_to_key_to_bucket_index.clear();
    first_to_keys_and_buckets.clear();
}

template<class Entry>
bool LinearWeightedTypeBasedOpenList<Entry>::is_dead_end(
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
bool LinearWeightedTypeBasedOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    for (const shared_ptr<Evaluator> &evaluator : evaluators) {
        if (evaluator->dead_ends_are_reliable() &&
            eval_context.is_evaluator_value_infinite(evaluator.get()))
            return true;
    }
    return false;
}

template<class Entry>
void LinearWeightedTypeBasedOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    for (const shared_ptr<Evaluator> &evaluator : evaluators) {
        evaluator->get_path_dependent_evaluators(evals);
    }
}

LinearWeightedTypeBasedOpenListFactory::LinearWeightedTypeBasedOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
LinearWeightedTypeBasedOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<LinearWeightedTypeBasedOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
LinearWeightedTypeBasedOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<LinearWeightedTypeBasedOpenList<EdgeOpenListEntry>>(options);
}

class LinearWeightedTypeBasedOpenListFeature : public plugins::TypedFeature<OpenListFactory, LinearWeightedTypeBasedOpenListFactory> {
public:
    LinearWeightedTypeBasedOpenListFeature(): TypedFeature("linear_weighted_type_based"){
	document_title("LinearWeightedType-based open list");
	document_synopsis(
	    "Uses multiple evaluators to assign entries to buckets. "
	    "All entries in a bucket have the same evaluator values. "
	    "When retrieving an entry, a bucket is chosen uniformly at "
	    "random and one of the contained entries is selected "
	    "uniformly randomly. "
	    "The algorithm is based on" + utils::format_conference_reference(
		{"Fan Xie", "Martin Mueller", "Robert Holte", "Tatsuya Imai"},
		"LinearWeightedType-Based Exploration with Multiple Search Queues for"
		" Satisficing Planning",
		"http://www.aaai.org/ocs/index.php/AAAI/AAAI14/paper/view/8472/8705",
		"Proceedings of the Twenty-Eigth AAAI Conference Conference"
		" on Artificial Intelligence (AAAI 2014)",
		"2395-2401",
		"AAAI Press",
		"2014"));
	add_list_option<shared_ptr<Evaluator>>(
	    "evaluators",
	    "Evaluators used to determine the bucket for each entry.");
	add_option<double>("alpha", "coefficient", "1.0");
	add_option<double>("beta", "bias", "1.0");
	add_option<bool>(
	    "ignore_size",
	    "ignore size of second to last keys", "false");
	add_option<bool>(
	    "ignore_weights",
	    "ignore linear_weighted weights", "false");

	utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<LinearWeightedTypeBasedOpenListFeature> _plugin;
}


