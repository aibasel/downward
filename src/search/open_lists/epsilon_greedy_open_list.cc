#include "epsilon_greedy_open_list.h"

#include "../evaluator.h"
#include "../open_list.h"

#include "../plugins/plugin.h"
#include "../utils/collections.h"
#include "../utils/markup.h"
#include "../utils/memory.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <functional>
#include <memory>

using namespace std;

namespace epsilon_greedy_open_list {
template<class Entry>
class EpsilonGreedyOpenList : public OpenList<Entry> {
    shared_ptr<utils::RandomNumberGenerator> rng;

    struct HeapNode {
        int id;
        int h;
        Entry entry;
        HeapNode(int id, int h, const Entry &entry)
            : id(id), h(h), entry(entry) {
        }

        bool operator>(const HeapNode &other) const {
            return make_pair(h, id) > make_pair(other.h, other.id);
        }
    };

    vector<HeapNode> heap;
    shared_ptr<Evaluator> evaluator;

    double epsilon;
    int size;
    int next_id;

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    EpsilonGreedyOpenList(
        const shared_ptr<Evaluator> &eval, double epsilon,
        int random_seed, bool pref_only);

    virtual Entry remove_min() override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;
    virtual bool empty() const override;
    virtual void clear() override;
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
void EpsilonGreedyOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    heap.emplace_back(
        next_id++, eval_context.get_evaluator_value(evaluator.get()), entry);
    push_heap(heap.begin(), heap.end(), greater<HeapNode>());
    ++size;
}

template<class Entry>
EpsilonGreedyOpenList<Entry>::EpsilonGreedyOpenList(
    const shared_ptr<Evaluator> &eval, double epsilon,
    int random_seed, bool pref_only)
    : OpenList<Entry>(pref_only),
      rng(utils::get_rng(random_seed)),
      evaluator(eval),
      epsilon(epsilon),
      size(0),
      next_id(0) {
}

template<class Entry>
Entry EpsilonGreedyOpenList<Entry>::remove_min() {
    assert(size > 0);
    if (rng->random() < epsilon) {
        int pos = rng->random(size);
        heap[pos].h = numeric_limits<int>::min();
        adjust_heap_up(heap, pos);
    }
    pop_heap(heap.begin(), heap.end(), greater<HeapNode>());
    HeapNode heap_node = heap.back();
    heap.pop_back();
    --size;
    return heap_node.entry;
}

template<class Entry>
bool EpsilonGreedyOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool EpsilonGreedyOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

template<class Entry>
void EpsilonGreedyOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool EpsilonGreedyOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void EpsilonGreedyOpenList<Entry>::clear() {
    heap.clear();
    size = 0;
    next_id = 0;
}

EpsilonGreedyOpenListFactory::EpsilonGreedyOpenListFactory(
    const shared_ptr<Evaluator> &eval, double epsilon,
    int random_seed, bool pref_only)
    : eval(eval),
      epsilon(epsilon),
      random_seed(random_seed),
      pref_only(pref_only) {
}

unique_ptr<StateOpenList>
EpsilonGreedyOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<EpsilonGreedyOpenList<StateOpenListEntry>>(
        eval, epsilon, random_seed, pref_only);
}

unique_ptr<EdgeOpenList>
EpsilonGreedyOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<EpsilonGreedyOpenList<EdgeOpenListEntry>>(
        eval, epsilon, random_seed, pref_only);
}

class EpsilonGreedyOpenListFeature
    : public plugins::TypedFeature<OpenListFactory, EpsilonGreedyOpenListFactory> {
public:
    EpsilonGreedyOpenListFeature() : TypedFeature("epsilon_greedy") {
        document_title("Epsilon-greedy open list");
        document_synopsis(
            "Chooses an entry uniformly randomly with probability "
            "'epsilon', otherwise it returns the minimum entry. "
            "The algorithm is based on" + utils::format_conference_reference(
                {"Richard Valenzano", "Nathan R. Sturtevant",
                 "Jonathan Schaeffer", "Fan Xie"},
                "A Comparison of Knowledge-Based GBFS Enhancements and"
                " Knowledge-Free Exploration",
                "http://www.aaai.org/ocs/index.php/ICAPS/ICAPS14/paper/view/7943/8066",
                "Proceedings of the Twenty-Fourth International Conference"
                " on Automated Planning and Scheduling (ICAPS 2014)",
                "375-379",
                "AAAI Press",
                "2014"));

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_option<double>(
            "epsilon",
            "probability for choosing the next entry randomly",
            "0.2",
            plugins::Bounds("0.0", "1.0"));
        utils::add_rng_options_to_feature(*this);
        add_open_list_options_to_feature(*this);
    }

    virtual shared_ptr<EpsilonGreedyOpenListFactory> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<EpsilonGreedyOpenListFactory>(
            opts.get<shared_ptr<Evaluator>>("eval"),
            opts.get<double>("epsilon"),
            utils::get_rng_arguments_from_options(opts),
            get_open_list_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<EpsilonGreedyOpenListFeature> _plugin;
}
