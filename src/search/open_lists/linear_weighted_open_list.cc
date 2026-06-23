#include "linear_weighted_open_list.h"

#include "../evaluator.h"
#include "../open_list.h"

#include "../plugins/plugin.h"
#include "../utils/memory.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <cassert>
#include <cmath>
#include <deque>
#include <map>

using namespace std;

namespace linear_weighted_open_list {
template<class Entry>
class LinearWeightedOpenList : public OpenList<Entry> {
    typedef deque<Entry> Bucket;

    utils::RandomNumberGenerator rng;
    map<int, Bucket> buckets;
    int size;
    double alpha;
    double beta;
    bool ignore_size;
    double epsilon;

    shared_ptr<Evaluator> evaluator;

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    explicit LinearWeightedOpenList(const plugins::Options &opts);
    LinearWeightedOpenList(const shared_ptr<Evaluator> &eval, bool preferred_only);
    virtual ~LinearWeightedOpenList() override = default;

    virtual Entry remove_min() override;
    virtual bool empty() override;
    virtual void clear() override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
};


template<class Entry>
LinearWeightedOpenList<Entry>::LinearWeightedOpenList(const plugins::Options &opts)
    : OpenList<Entry>(opts.get<bool>("pref_only")),
      rng(opts.get<int>("random_seed")),
      size(0),
      alpha(opts.get<double>("alpha")),
      beta(opts.get<double>("beta")),
      ignore_size(opts.get<bool>("ignore_size")),
      epsilon(opts.get<double>("epsilon")),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

template<class Entry>
void LinearWeightedOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    int key = eval_context.get_evaluator_value(evaluator.get());

    buckets[key].push_back(entry);
    ++size;
}

template<class Entry>
Entry LinearWeightedOpenList<Entry>::remove_min() {
    assert(size > 0);
    int key = buckets.begin()->first;

    if (buckets.size() > 1) {
        double r = rng.random();
        if (r <= epsilon) {
            double total_sum = 0;
            double bias = beta + alpha * static_cast<double>(buckets.rbegin()->first);
            for (auto it : buckets) {
                double s = -1.0 * alpha * static_cast<double>(it.first) + bias;
                if (!ignore_size) s *= static_cast<double>(it.second.size());
                total_sum += s;
            }
            double p_sum = 0.0;
            for (auto it : buckets) {
                double p = (-1.0 * alpha * static_cast<double>(it.first) + bias) / total_sum;
                if (!ignore_size) p *= static_cast<double>(it.second.size());
                p_sum += p;
                if (r <= p_sum * epsilon) {
                    key = it.first;
                    break;
                }
            }
        }
    }

    Bucket &bucket = buckets[key];
    assert(!bucket.empty());
    Entry result = bucket.front();
    bucket.pop_front();
    if (bucket.empty()) buckets.erase(key);
    --size;
    return result;
}

template<class Entry>
bool LinearWeightedOpenList<Entry>::empty() {
    return size == 0;
}

template<class Entry>
void LinearWeightedOpenList<Entry>::clear() {
    buckets.clear();
    size = 0;
}

template<class Entry>
void LinearWeightedOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool LinearWeightedOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool LinearWeightedOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

LinearWeightedOpenListFactory::LinearWeightedOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
LinearWeightedOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<LinearWeightedOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
LinearWeightedOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<LinearWeightedOpenList<EdgeOpenListEntry>>(options);
}

class LinearWeightedOpenListFeature : public plugins::TypedFeature<OpenListFactory, LinearWeightedOpenListFactory> {
public:
    LinearWeightedOpenListFeature(): TypedFeature("linear_weighted") {
	document_title("LinearWeighted open list");
	document_synopsis(
	    "Open list that uses a single evaluator and FIFO tiebreaking.");
	document_note(
	    "Implementation Notes",
	    "Elements with the same evaluator value are stored in double-ended "
	    "queues, called \"buckets\". The open list stores a map from evaluator "
	    "values to buckets. Pushing and popping from a bucket runs in constant "
	    "time. Therefore, inserting and removing an entry from the open list "
	    "takes time O(log(n)), where n is the number of buckets.");
	add_option<shared_ptr<Evaluator>>("eval", "evaluator");
	add_option<bool>(
	    "pref_only",
	    "insert only nodes generated by preferred operators", "false");
	add_option<double>("alpha", "coefficent", "1.0");
	add_option<double>("beta", "bias", "1.0");
	add_option<bool>(
	    "ignore_size",
	    "ignore bucket sizes", "false");
	add_option<double>(
	    "epsilon",
	    "probability for choosing the next entry randomly",
	    "1.0",
	    plugins::Bounds("0.0", "1.0"));

	utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<LinearWeightedOpenListFeature> _plugin;
}



