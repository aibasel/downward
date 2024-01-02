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
AlternationOpenListFactory::AlternationOpenListFactory(vector<shared_ptr<OpenListFactory>> open_list_factories,
                                                       int boost_amount)
    : boost_amount(boost_amount), size(0), open_list_factories(open_list_factories) {
}


unique_ptr<StateOpenList>
AlternationOpenListFactory::create_state_open_list() {
    return make_unique<AlternationOpenList<StateOpenListEntry>>(open_list_factories, boost_amount);
}


unique_ptr<EdgeOpenList>
AlternationOpenListFactory::create_edge_open_list() {
    return make_unique<AlternationOpenList<EdgeOpenListEntry>>(open_list_factories, boost_amount);
}


TaskIndependentAlternationOpenListFactory::TaskIndependentAlternationOpenListFactory(
    vector<shared_ptr<TaskIndependentOpenListFactory>> open_list_factories,
    int boost_amount
    )
    : TaskIndependentOpenListFactory("AlternationOpenListFactory", utils::Verbosity::NORMAL),
      boost_amount(boost_amount), size(0), open_list_factories(open_list_factories) {
}


using ConcreteProduct = AlternationOpenListFactory;
using AbstractProduct = OpenListFactory;
using Concrete = TaskIndependentAlternationOpenListFactory;
// TODO issue559 use templates as 'get_task_specific' is EXACTLY the same for all TI_Components
shared_ptr<AbstractProduct> Concrete::get_task_specific(
    [[maybe_unused]] const std::shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map,
    int depth) const {
    shared_ptr<ConcreteProduct> task_specific_x;

    if (component_map->count(static_cast<const TaskIndependentComponent *>(this))) {
        log << std::string(depth, ' ') << "Reusing task specific " << get_product_name() << " '" << name << "'..." << endl;
        task_specific_x = dynamic_pointer_cast<ConcreteProduct>(
            component_map->at(static_cast<const TaskIndependentComponent *>(this)));
    } else {
        log << std::string(depth, ' ') << "Creating task specific " << get_product_name() << " '" << name << "'..." << endl;
        task_specific_x = create_ts(task, component_map, depth);
        component_map->insert(make_pair<const TaskIndependentComponent *, std::shared_ptr<Component>>
                                  (static_cast<const TaskIndependentComponent *>(this), task_specific_x));
    }
    return task_specific_x;
}

std::shared_ptr<ConcreteProduct> Concrete::create_ts(const shared_ptr <AbstractTask> &task,
                                                     unique_ptr <ComponentMap> &component_map, int depth) const {
    vector<shared_ptr<OpenListFactory>> td_open_list_factories(open_list_factories.size());
    transform(open_list_factories.begin(), open_list_factories.end(), td_open_list_factories.begin(),
              [this, &task, &component_map, &depth](const shared_ptr<TaskIndependentOpenListFactory> &eval) {
                  return eval->get_task_specific(task, component_map, depth >= 0 ? depth + 1 : depth);
              }
              );

    return make_shared<AlternationOpenListFactory>(td_open_list_factories, boost_amount);
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
        return make_shared<TaskIndependentAlternationOpenListFactory>(
            opts.get_list<shared_ptr<TaskIndependentOpenListFactory>>("sublists"),
            opts.get<int>("boost"));
    }
};

static plugins::FeaturePlugin<AlternationOpenListFeature> _plugin;
}
