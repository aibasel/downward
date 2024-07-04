#include "tiebreaking_open_list.h"

#include "../evaluator.h"
#include "../open_list.h"

#include "../plugins/plugin.h"
#include "../utils/memory.h"

#include <cassert>
#include <deque>
#include <map>
#include <utility>
#include <vector>

using namespace std;

namespace tiebreaking_open_list {
template<class Entry>
class TieBreakingOpenList : public OpenList<Entry> {
    using Bucket = deque<Entry>;

    map<const vector<int>, Bucket> buckets;
    int size;

    vector<shared_ptr<Evaluator>> evaluators;
    /*
      If allow_unsafe_pruning is true, we ignore (don't insert) states
      which the first evaluator considers a dead end, even if it is
      not a safe heuristic.
    */
    bool allow_unsafe_pruning;

    int dimension() const;

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    TieBreakingOpenList(
        const vector<shared_ptr<Evaluator>> &evals,
        bool unsafe_pruning, bool pref_only);

    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
};


template<class Entry>
TieBreakingOpenList<Entry>::TieBreakingOpenList(
    const vector<shared_ptr<Evaluator>> &evals,
    bool unsafe_pruning, bool pref_only)
    : OpenList<Entry>(pref_only),
      size(0), evaluators(evals),
      allow_unsafe_pruning(unsafe_pruning) {
}

template<class Entry>
void TieBreakingOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    vector<int> key;
    key.reserve(evaluators.size());
    for (const shared_ptr<Evaluator> &evaluator : evaluators)
        key.push_back(eval_context.get_evaluator_value_or_infinity(evaluator.get()));

    buckets[key].push_back(entry);
    ++size;
}

template<class Entry>
Entry TieBreakingOpenList<Entry>::remove_min() {
    assert(size > 0);
    typename map<const vector<int>, Bucket>::iterator it;
    it = buckets.begin();
    assert(it != buckets.end());
    assert(!it->second.empty());
    --size;
    Entry result = it->second.front();
    it->second.pop_front();
    if (it->second.empty())
        buckets.erase(it);
    return result;
}

template<class Entry>
bool TieBreakingOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void TieBreakingOpenList<Entry>::clear() {
    buckets.clear();
    size = 0;
}

template<class Entry>
int TieBreakingOpenList<Entry>::dimension() const {
    return evaluators.size();
}

template<class Entry>
void TieBreakingOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    for (const shared_ptr<Evaluator> &evaluator : evaluators)
        evaluator->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool TieBreakingOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    // TODO: Properly document this behaviour.
    // If one safe heuristic detects a dead end, return true.
    if (is_reliable_dead_end(eval_context))
        return true;
    // If the first heuristic detects a dead-end and we allow "unsafe
    // pruning", return true.
    if (allow_unsafe_pruning &&
        eval_context.is_evaluator_value_infinite(evaluators[0].get()))
        return true;
    // Otherwise, return true if all heuristics agree this is a dead-end.
    for (const shared_ptr<Evaluator> &evaluator : evaluators)
        if (!eval_context.is_evaluator_value_infinite(evaluator.get()))
            return false;
    return true;
}

template<class Entry>
bool TieBreakingOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    for (const shared_ptr<Evaluator> &evaluator : evaluators)
        if (eval_context.is_evaluator_value_infinite(evaluator.get()) &&
            evaluator->dead_ends_are_reliable())
            return true;
    return false;
}

TieBreakingOpenListFactory::TieBreakingOpenListFactory(
    const vector<shared_ptr<Evaluator>> &evals,
    bool unsafe_pruning, bool pref_only)
    : evals(evals),
      unsafe_pruning(unsafe_pruning),
      pref_only(pref_only) {
}

unique_ptr<StateOpenList>
TieBreakingOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<TieBreakingOpenList<StateOpenListEntry>>(
        evals, unsafe_pruning, pref_only);
}

unique_ptr<EdgeOpenList>
TieBreakingOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<TieBreakingOpenList<EdgeOpenListEntry>>(
        evals, unsafe_pruning, pref_only);
}

class TieBreakingOpenListFeature
    : public plugins::TypedFeature<OpenListFactory, TieBreakingOpenListFactory> {
public:
    TieBreakingOpenListFeature() : TypedFeature("tiebreaking") {
        document_title("Tie-breaking open list");
        document_synopsis("");

        add_list_option<shared_ptr<Evaluator>>("evals", "evaluators");
        add_option<bool>(
            "unsafe_pruning",
            "allow unsafe pruning when the main evaluator regards a state a dead end",
            "true");
        add_open_list_options_to_feature(*this);
    }

    virtual shared_ptr<TieBreakingOpenListFactory> create_component(
        const plugins::Options &opts,
        const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<Evaluator>>(
            context, opts, "evals");
        return plugins::make_shared_from_arg_tuples<TieBreakingOpenListFactory>(
            opts.get_list<shared_ptr<Evaluator>>("evals"),
            opts.get<bool>("unsafe_pruning"),
            get_open_list_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<TieBreakingOpenListFeature> _plugin;
}
