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
TieBreakingOpenListFactory::TieBreakingOpenListFactory(const plugins::Options &opts)
    : options(opts),
    pref_only(opts.get<bool>("pref_only")),
    size(0),
    evaluators(opts.get_list<shared_ptr<Evaluator>>("evals")),
    allow_unsafe_pruning(opts.get<bool>("unsafe_pruning")) {
}

TieBreakingOpenListFactory::TieBreakingOpenListFactory(
    bool pref_only,
    vector<shared_ptr<Evaluator>> evaluators,
    bool allow_unsafe_pruning)
    : pref_only(pref_only), size(0), evaluators(evaluators), allow_unsafe_pruning(allow_unsafe_pruning) {
}

shared_ptr<StateOpenList>
TieBreakingOpenListFactory::create_state_open_list() {
    return make_shared<TieBreakingOpenList<StateOpenListEntry>>(pref_only, evaluators, allow_unsafe_pruning);
}

shared_ptr<EdgeOpenList>
TieBreakingOpenListFactory::create_edge_open_list() {
    return make_shared<TieBreakingOpenList<EdgeOpenListEntry>>(pref_only, evaluators, allow_unsafe_pruning);
}

TaskIndependentTieBreakingOpenListFactory::TaskIndependentTieBreakingOpenListFactory(const plugins::Options &opts)
    : options(opts), pref_only(opts.get<bool>("pref_only")), size(0), evaluators(opts.get_list<shared_ptr<TaskIndependentEvaluator>>("evals")), allow_unsafe_pruning(opts.get<bool>("unsafe_pruning")) {
}

TaskIndependentTieBreakingOpenListFactory::TaskIndependentTieBreakingOpenListFactory(
    bool pref_only,
    vector<shared_ptr<TaskIndependentEvaluator>> evaluators,
    bool allow_unsafe_pruning)
    : pref_only(pref_only), size(0), evaluators(evaluators), allow_unsafe_pruning(allow_unsafe_pruning) {
}




shared_ptr<TieBreakingOpenListFactory> TaskIndependentTieBreakingOpenListFactory::create_task_specific_TieBreakingOpenListFactory(const shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<TieBreakingOpenListFactory> task_specific_x;
    utils::g_log << std::string(depth, ' ') << "Checking ComponentMap for TieBreakingOpenListFactory..." << endl;
    utils::g_log << std::string(depth, ' ') << "Checking ComponentMap for TieBreakingOpenListFactory..." << endl;

    if (component_map->contains_key(make_pair(task, static_cast<void *>(this)))) {
        utils::g_log << std::string(depth, ' ') << "Reusing task specific EagerSearch..." << endl;
        task_specific_x = plugins::any_cast<shared_ptr<TieBreakingOpenListFactory>>(
            component_map->get_dual_key_value(task, this));
    } else {
        utils::g_log << std::string(depth, ' ') << "Creating task specific TieBreakingOpenListFactory..." << endl;
        vector<shared_ptr<Evaluator>> ts_evaluators(evaluators.size());

        transform(evaluators.begin(), evaluators.end(), ts_evaluators.begin(),
                  [this, &task, &component_map, &depth](const shared_ptr<TaskIndependentEvaluator> &eval) {
                      return eval->create_task_specific_Evaluator(task, component_map, depth >=0 ? depth+1 : depth);
                  }
                  );

        task_specific_x = make_shared<TieBreakingOpenListFactory>(pref_only,
                                                                  ts_evaluators,
                                                                  allow_unsafe_pruning);
        component_map->add_dual_key_entry(task, this, plugins::Any(task_specific_x));
    }
    return task_specific_x;
}


shared_ptr<TieBreakingOpenListFactory> TaskIndependentTieBreakingOpenListFactory::create_task_specific_TieBreakingOpenListFactory(const shared_ptr<AbstractTask> &task, int depth) {
    utils::g_log << std::string(depth, ' ') << "Creating TieBreakingOpenListFactory as root component..." << endl;
    std::shared_ptr<ComponentMap> component_map = std::make_shared<ComponentMap>();
    return create_task_specific_TieBreakingOpenListFactory(task, component_map, depth);
}


shared_ptr<OpenListFactory> TaskIndependentTieBreakingOpenListFactory::create_task_specific_OpenListFactory(const shared_ptr<AbstractTask> &task, int depth) {
    std::shared_ptr<ComponentMap> component_map = std::make_shared<ComponentMap>();
    return create_task_specific_OpenListFactory(task, component_map, depth);
}

shared_ptr<OpenListFactory> TaskIndependentTieBreakingOpenListFactory::create_task_specific_OpenListFactory(const shared_ptr<AbstractTask> &task, shared_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<TieBreakingOpenListFactory> x = create_task_specific_TieBreakingOpenListFactory(task, component_map, depth);
    return static_pointer_cast<OpenListFactory>(x);
}

/* Job of the specific factory
unique_ptr<TaskIndependentStateOpenList>
TaskIndependentTieBreakingOpenListFactory::create_task_independent_state_open_list() {
    return utils::make_unique_ptr<TaskIndependentTieBreakingOpenList<StateOpenListEntry>>(pref_only, evaluators, allow_unsafe_pruning);
}

unique_ptr<TaskIndependentEdgeOpenList>
TaskIndependentTieBreakingOpenListFactory::create_task_independent_edge_open_list() {
    return utils::make_unique_ptr<TaskIndependentTieBreakingOpenList<EdgeOpenListEntry>>(pref_only, evaluators, allow_unsafe_pruning);
}
*/

class TieBreakingOpenListFeature : public plugins::TypedFeature<TaskIndependentOpenListFactory, TaskIndependentTieBreakingOpenListFactory> {
public:
    TieBreakingOpenListFeature() : TypedFeature("tiebreaking") {
        document_title("Tie-breaking open list");
        document_synopsis("");

        add_list_option<shared_ptr<TaskIndependentEvaluator>>("evals", "evaluators");
        add_option<bool>(
            "pref_only",
            "insert only nodes generated by preferred operators", "false");
        add_option<bool>(
            "unsafe_pruning",
            "allow unsafe pruning when the main evaluator regards a state a dead end",
            "true");
    }

    virtual shared_ptr<TaskIndependentTieBreakingOpenListFactory> create_component(const plugins::Options &opts, const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<TaskIndependentEvaluator>>(context, opts, "evals");
        return make_shared<TaskIndependentTieBreakingOpenListFactory>(opts.get<bool>("pref_only"),
                                                                      opts.get_list<shared_ptr<TaskIndependentEvaluator>>("evals"),
                                                                      opts.get<bool>("unsafe_pruning"));
    }
};

static plugins::FeaturePlugin<TieBreakingOpenListFeature> _plugin;
}
