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
AlternationOpenListFactory::AlternationOpenListFactory(const plugins::Options &opts)
        : options(opts), boost_amount(opts.get<int>("boost")), size(0), open_list_factories(opts.get_list<shared_ptr<OpenListFactory>>("sublists")) {
}

AlternationOpenListFactory::AlternationOpenListFactory(int boost_amount,vector<shared_ptr<OpenListFactory>> open_list_factories)
        : boost_amount(boost_amount), size(0), open_list_factories(open_list_factories) {
}


unique_ptr<StateOpenList>
AlternationOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<AlternationOpenList<StateOpenListEntry>>(boost_amount, open_list_factories);
}


unique_ptr<EdgeOpenList>
AlternationOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<AlternationOpenList<EdgeOpenListEntry>>(boost_amount, open_list_factories);
}



TaskIndependentAlternationOpenListFactory::TaskIndependentAlternationOpenListFactory(const plugins::Options &opts)
        : options(opts), boost_amount(opts.get<int>("boost")), size(0), open_list_factories(opts.get_list<shared_ptr<TaskIndependentOpenListFactory>>("sublists")) {
}

TaskIndependentAlternationOpenListFactory::TaskIndependentAlternationOpenListFactory(
        int  boost_amount,
        vector<shared_ptr<TaskIndependentOpenListFactory>> open_list_factories
)
        : boost_amount(boost_amount), size(0), open_list_factories(open_list_factories) {
}



    std::unique_ptr<TaskIndependentStateOpenList>
    TaskIndependentAlternationOpenListFactory::create_task_independent_state_open_list() {
        return utils::make_unique_ptr<TaskIndependentAlternationOpenList<StateOpenListEntry>>(boost_amount, open_list_factories);
    }

    std::unique_ptr<TaskIndependentEdgeOpenList>
    TaskIndependentAlternationOpenListFactory::create_task_independent_edge_open_list() {
        return utils::make_unique_ptr<TaskIndependentAlternationOpenList<EdgeOpenListEntry>>(boost_amount, open_list_factories);
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


class AlternationOpenListFeature : public plugins::TypedFeature<TaskIndependentOpenListFactory, TaskIndependentAlternationOpenListFactory> {
public:
    AlternationOpenListFeature() : TypedFeature("alt") {
        document_title("Alternation open list");
        document_synopsis(
                "alternates between several open lists.");

        add_list_option<shared_ptr<TaskIndependentOpenListFactory>>(
                "sublists",
                "open lists between which this one alternates");
        add_option<int>(
                "boost",
                "boost value for contained open lists that are restricted "
                "to preferred successors",
                "0");
    }

    virtual shared_ptr<TaskIndependentAlternationOpenListFactory> create_component(const plugins::Options &opts, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<TaskIndependentOpenListFactory>>(context, opts, "sublists");
        return make_shared<TaskIndependentAlternationOpenListFactory>(opts.get<int>("boost"),
                                                                      opts.get_list<shared_ptr<TaskIndependentOpenListFactory>>("sublists"));
    }
};

static plugins::FeaturePlugin<AlternationOpenListFeature> _plugin;
}
