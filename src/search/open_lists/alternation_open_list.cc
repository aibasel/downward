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

AlternationOpenListFactory::AlternationOpenListFactory(int boost_amount, vector<shared_ptr<OpenListFactory>> open_list_factories)
    : boost_amount(boost_amount), size(0), open_list_factories(open_list_factories) {
}


shared_ptr<StateOpenList>
AlternationOpenListFactory::create_state_open_list() {
    return make_shared<AlternationOpenList<StateOpenListEntry>>(boost_amount, open_list_factories);
}


shared_ptr<EdgeOpenList>
AlternationOpenListFactory::create_edge_open_list() {
    return make_shared<AlternationOpenList<EdgeOpenListEntry>>(boost_amount, open_list_factories);
}



TaskIndependentAlternationOpenListFactory::TaskIndependentAlternationOpenListFactory(const plugins::Options &opts)
    : options(opts), boost_amount(opts.get<int>("boost")), size(0), open_list_factories(opts.get_list<shared_ptr<TaskIndependentOpenListFactory>>("sublists")) {
}

TaskIndependentAlternationOpenListFactory::TaskIndependentAlternationOpenListFactory(
    int boost_amount,
    vector<shared_ptr<TaskIndependentOpenListFactory>> open_list_factories
    )
    : boost_amount(boost_amount), size(0), open_list_factories(open_list_factories) {
}


shared_ptr<AlternationOpenListFactory> TaskIndependentAlternationOpenListFactory::create_task_specific_AlternationOpenListFactory(
        const std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<AlternationOpenListFactory> task_specific_x;
    if (component_map->contains_key(make_pair(task, static_cast<void *>(this)))) {
        utils::g_log << std::string(depth, ' ') << "Reusing task AlternationOpenListFactory..." << endl;
        task_specific_x = plugins::any_cast<shared_ptr<AlternationOpenListFactory>>(
            component_map->get_dual_key_value(task, this));
    } else {
        utils::g_log << std::string(depth, ' ') << "Creating task specific AlternationOpenListFactory..." << endl;

        vector<shared_ptr<OpenListFactory>> td_open_list_factories(open_list_factories.size());
        transform(open_list_factories.begin(), open_list_factories.end(), td_open_list_factories.begin(),
                  [this, &task, &component_map, &depth](const shared_ptr<TaskIndependentOpenListFactory> &eval) {
                      return eval->create_task_specific_OpenListFactory(task, component_map, depth >=0 ? depth+1 : depth);
                  }
                  );

        task_specific_x = make_shared<AlternationOpenListFactory>(boost_amount, td_open_list_factories);
        component_map->add_dual_key_entry(task, this, plugins::Any(task_specific_x));
    }
    return task_specific_x;
}


shared_ptr<AlternationOpenListFactory> TaskIndependentAlternationOpenListFactory::create_task_specific_AlternationOpenListFactory(const shared_ptr<AbstractTask> &task, int depth) {
    utils::g_log << std::string(depth, ' ') << "Creating AlternationOpenListFactory as root component..." << endl;
    std::shared_ptr<ComponentMap> component_map = std::make_shared<ComponentMap>();
    return create_task_specific_AlternationOpenListFactory(task, component_map, depth);
}



shared_ptr<OpenListFactory> TaskIndependentAlternationOpenListFactory::create_task_specific_OpenListFactory(const shared_ptr<AbstractTask> &task, shared_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<AlternationOpenListFactory> x = create_task_specific_AlternationOpenListFactory(task, component_map, depth);
    return static_pointer_cast<OpenListFactory>(x);
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
