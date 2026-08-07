#include "linear_weighted_heap_open_list.h"

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

namespace linear_weighted_heap_open_list {
template<class Entry>
class LinearWeightedHeapOpenList : public OpenList<Entry> {
    struct HeapNode {
        int id;
        Entry entry;
        HeapNode(int id, const Entry &entry)
            : id(id), entry(entry) {
        }

        bool operator>(const HeapNode &other) const {
            return id > other.id;
        }
    };

    typedef vector<HeapNode> Bucket;

    utils::RandomNumberGenerator rng;
    map<int, Bucket> buckets;
    int size;
    double alpha;
    double beta;
    bool ignore_size;
    bool ignore_weights;
    double epsilon;
    bool only_tie_breaking;
    int next_id;

    shared_ptr<Evaluator> evaluator;

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    explicit LinearWeightedHeapOpenList(const plugins::Options &opts);
    LinearWeightedHeapOpenList(const shared_ptr<Evaluator> &eval, bool preferred_only);
    virtual ~LinearWeightedHeapOpenList() override = default;

    virtual Entry remove_min() override;
    virtual bool empty() override;
    virtual void clear() override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
};

template<class HeapNode>
static void adjust_heap_up(vector<HeapNode> &heap, size_t pos) {
    assert(utils::in_bounds(pos, heap));
    while (pos != 0) {
        size_t parent_pos = (pos - 1) / 2;
        if (heap[pos] > heap[parent_pos]) {
            break;
        }
        swap(heap[pos], heap[parent_pos]);
        pos = parent_pos;
    }
}


template<class Entry>
LinearWeightedHeapOpenList<Entry>::LinearWeightedHeapOpenList(const plugins::Options &opts)
    : OpenList<Entry>(opts.get<bool>("pref_only")),
      rng(opts.get<int>("random_seed")),
      size(0),
      alpha(opts.get<double>("alpha")),
      beta(opts.get<double>("beta")),
      ignore_size(opts.get<bool>("ignore_size")),
      ignore_weights(opts.get<bool>("ignore_weights")),
      epsilon(opts.get<double>("epsilon")),
      only_tie_breaking(opts.get<bool>("only_tie_breaking")),
      next_id(0),
      evaluator(opts.get<shared_ptr<Evaluator>>("eval")) {
}

template<class Entry>
void LinearWeightedHeapOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    int key = eval_context.get_evaluator_value(evaluator.get());

    buckets[key].emplace_back(next_id++, entry);
    push_heap(buckets[key].begin(), buckets[key].end(), greater<HeapNode>());
    ++size;
}

template<class Entry>
Entry LinearWeightedHeapOpenList<Entry>::remove_min() {
    assert(size > 0);
    int key = buckets.begin()->first;
    bool random_tie_breaking = false;

    double r = rng.random();
    if (r <= epsilon) {
        random_tie_breaking = true;
        if (!only_tie_breaking && buckets.size() > 1) {
            double bias = static_cast<double>(buckets.rbegin()->first) + beta;
            double p_sum = 0.0;
            double total_sum = 0.0;
            for (auto it : buckets) {
                double v = 0;
                if (ignore_size)
                    v = 1;
                else
                    v = it.second.size();
                if (!ignore_weights) v *= -1 * alpha * static_cast<double>(it.first) + bias;
                total_sum += v;
            }
            for (auto it : buckets) {
                double p = 1.0 / total_sum;
                if (!ignore_weights) p *= -1 * alpha * static_cast<double>(it.first) + bias;
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

    if (random_tie_breaking) {
        int pos = rng.random(bucket.size());
        bucket[pos].id = numeric_limits<int>::min();
        adjust_heap_up(bucket, pos);
    }

    pop_heap(bucket.begin(), bucket.end(), greater<HeapNode>());
    HeapNode heap_node = bucket.back();
    bucket.pop_back();

    if (bucket.empty()) buckets.erase(key);

    --size;
    return heap_node.entry;
}

template<class Entry>
bool LinearWeightedHeapOpenList<Entry>::empty() {
    return size == 0;
}

template<class Entry>
void LinearWeightedHeapOpenList<Entry>::clear() {
    buckets.clear();
    size = 0;
}

template<class Entry>
void LinearWeightedHeapOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool LinearWeightedHeapOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool LinearWeightedHeapOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

LinearWeightedHeapOpenListFactory::LinearWeightedHeapOpenListFactory(
    const plugins::Options &options)
    : options(options) {
}

unique_ptr<StateOpenList>
LinearWeightedHeapOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<LinearWeightedHeapOpenList<StateOpenListEntry>>(options);
}

unique_ptr<EdgeOpenList>
LinearWeightedHeapOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<LinearWeightedHeapOpenList<EdgeOpenListEntry>>(options);
}

class LinearWeightedHeapOpenListFeature : public plugins::TypedFeature<OpenListFactory, LinearWeightedHeapOpenListFactory> {
public:
    LinearWeightedHeapOpenListFeature() : TypedFeature("linear_weighted_heap") {

        document_title("LinearWeightedHeap open list");
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
	add_option<bool>(
	    "ignore_weights",
	    "ignore weights of buckets", "false");
	add_option<double>(
	    "epsilon",
	    "probability for choosing the next entry randomly",
	    "1.0",
	    plugins::Bounds("0.0", "1.0"));
	add_option<bool>(
	    "only_tie_breaking",
	    "just randomize tie-breaking", "false");

	utils::add_rng_options(*this);
    }
};

static plugins::FeaturePlugin<LinearWeightedHeapOpenListFeature> _plugin;
}




