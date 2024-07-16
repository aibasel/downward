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
TaskIndependentTieBreakingOpenListFactory::TaskIndependentTieBreakingOpenListFactory(
    vector<shared_ptr<TaskIndependentEvaluator>> evals,
    bool pref_only,
    bool allow_unsafe_pruning)
    : TaskIndependentOpenListFactory("TieBreakingOpenListFactory", utils::Verbosity::NORMAL),
      pref_only(pref_only), size(0), evals(evals), allow_unsafe_pruning(allow_unsafe_pruning) {
}

std::shared_ptr<OpenListFactory> TaskIndependentTieBreakingOpenListFactory::create_ts(const shared_ptr <AbstractTask> &task,
                                                                                      unique_ptr <ComponentMap> &component_map, int depth) const {
    vector<shared_ptr<Evaluator>> ts_evaluators(evals.size());

    transform(evals.begin(), evals.end(), ts_evaluators.begin(),
              [this, &task, &component_map, &depth](const shared_ptr<TaskIndependentEvaluator> &eval) {
                  return eval->get_task_specific(task, component_map, depth >= 0 ? depth + 1 : depth);
              }
              );
    return make_shared<TieBreakingOpenListFactory>(
        ts_evaluators,
        pref_only,
        allow_unsafe_pruning);
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
    return make_unique<TieBreakingOpenList<EdgeOpenListEntry>>(
        evals, unsafe_pruning, pref_only);
}

class TieBreakingOpenListFeature
    : public plugins::TypedFeature<TaskIndependentOpenListFactory, TaskIndependentTieBreakingOpenListFactory> {
public:
    TieBreakingOpenListFeature() : TypedFeature("tiebreaking") {
        document_title("Tie-breaking open list");
        document_synopsis("");

        add_list_option<shared_ptr<TaskIndependentEvaluator>>("evals", "evaluators");
        add_option<bool>(
            "unsafe_pruning",
            "allow unsafe pruning when the main evaluator regards a state a dead end",
            "true");
        add_open_list_options_to_feature(*this);
    }

    virtual shared_ptr<TaskIndependentTieBreakingOpenListFactory> create_component(
        const plugins::Options &opts,
        const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<TaskIndependentEvaluator>>(
            context, opts, "evals");
        return plugins::make_shared_from_arg_tuples<TaskIndependentTieBreakingOpenListFactory>(
            opts.get_list<shared_ptr<TaskIndependentEvaluator>>("evals"),
            opts.get<bool>("unsafe_pruning"),
            get_open_list_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<TieBreakingOpenListFeature> _plugin;
}
