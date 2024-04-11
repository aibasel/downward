#include "alternation_open_list.h"

#include "../open_list.h"

#include "../plugins/plugin.h"
#include "../utils/memory.h"
#include "../utils/system.h"

#include <cassert>
#include <memory>
#include <vector>

using namespace std;
using utils::ExitCode;

namespace alternation_open_list {
template<class Entry>
class AlternationOpenList : public OpenList<Entry> {
    vector<unique_ptr<OpenList<Entry>>> open_lists;
    vector<int> priorities;

    const int boost_amount;
protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override;

public:
    AlternationOpenList(
        const vector<shared_ptr<OpenListFactory>> &sublists, int boost);

    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual void boost_preferred() override;
    virtual void get_path_dependent_evaluators(
        set<Evaluator *> &evals) override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
};


template<class Entry>
AlternationOpenList<Entry>::AlternationOpenList(
    const vector<shared_ptr<OpenListFactory>> &sublists, int boost)
    : boost_amount(boost) {
    vector<shared_ptr<OpenListFactory>> open_list_factories(sublists);
    open_lists.reserve(open_list_factories.size());
    for (const auto &factory : open_list_factories)
        open_lists.push_back(factory->create_open_list<Entry>());

    priorities.resize(open_lists.size(), 0);
}

template<class Entry>
void AlternationOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    for (const auto &sublist : open_lists)
        sublist->insert(eval_context, entry);
}

template<class Entry>
Entry AlternationOpenList<Entry>::remove_min() {
    int best = -1;
    for (size_t i = 0; i < open_lists.size(); ++i) {
        if (!open_lists[i]->empty() &&
            (best == -1 || priorities[i] < priorities[best])) {
            best = i;
        }
    }
    assert(best != -1);
    const auto &best_list = open_lists[best];
    assert(!best_list->empty());
    ++priorities[best];
    return best_list->remove_min();
}

template<class Entry>
bool AlternationOpenList<Entry>::empty() const {
    for (const auto &sublist : open_lists)
        if (!sublist->empty())
            return false;
    return true;
}

template<class Entry>
void AlternationOpenList<Entry>::clear() {
    for (const auto &sublist : open_lists)
        sublist->clear();
}

template<class Entry>
void AlternationOpenList<Entry>::boost_preferred() {
    for (size_t i = 0; i < open_lists.size(); ++i)
        if (open_lists[i]->only_contains_preferred_entries())
            priorities[i] -= boost_amount;
}

template<class Entry>
void AlternationOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    for (const auto &sublist : open_lists)
        sublist->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool AlternationOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    // If one sublist is sure we have a dead end, return true.
    if (is_reliable_dead_end(eval_context))
        return true;
    // Otherwise, return true if all sublists agree this is a dead-end.
    for (const auto &sublist : open_lists)
        if (!sublist->is_dead_end(eval_context))
            return false;
    return true;
}

template<class Entry>
bool AlternationOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    for (const auto &sublist : open_lists)
        if (sublist->is_reliable_dead_end(eval_context))
            return true;
    return false;
}


AlternationOpenListFactory::AlternationOpenListFactory(
    const vector<shared_ptr<OpenListFactory>> &sublists, int boost)
    : sublists(sublists),
      boost(boost) {
}

unique_ptr<StateOpenList>
AlternationOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<AlternationOpenList<StateOpenListEntry>>(
        sublists, boost);
}

unique_ptr<EdgeOpenList>
AlternationOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<AlternationOpenList<EdgeOpenListEntry>>(
        sublists, boost);
}

class AlternationOpenListFeature
    : public plugins::TypedFeature<OpenListFactory, AlternationOpenListFactory> {
public:
    AlternationOpenListFeature() : TypedFeature("alt") {
        document_title("Alternation open list");
        document_synopsis(
            "alternates between several open lists.");

        add_list_option<shared_ptr<OpenListFactory>>(
            "sublists",
            "open lists between which this one alternates");
        add_option<int>(
            "boost",
            "boost value for contained open lists that are restricted "
            "to preferred successors",
            "0");
    }

    virtual shared_ptr<AlternationOpenListFactory> create_component(
        const plugins::Options &opts,
        const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<OpenListFactory>>(
            context, opts, "sublists");
        return plugins::make_shared_from_arg_tuples<AlternationOpenListFactory>(
            opts.get_list<shared_ptr<OpenListFactory>>("sublists"),
            opts.get<int>("boost")
            );
    }
};

static plugins::FeaturePlugin<AlternationOpenListFeature> _plugin;
}
