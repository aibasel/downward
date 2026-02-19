#include "cost_adapted_heuristic.h"

#include "../plugins/plugin.h"
#include "../tasks/cost_adapted_task.h"

using namespace std;

namespace cost_adapted_heuristic {
shared_ptr<Evaluator>
TaskIndependentCostAdaptedHeuristic::create_task_specific_component(
    const shared_ptr<AbstractTask> &task, Cache &cache) const {
    shared_ptr<AbstractTask> cost_adapted_task =
        make_shared<tasks::CostAdaptedTask>(task, cost_type);
    return nested->bind_task(cost_adapted_task, cache);
}

TaskIndependentCostAdaptedHeuristic::TaskIndependentCostAdaptedHeuristic(
    shared_ptr<TaskIndependentEvaluator> nested, OperatorCost cost_type)
    : nested(nested), cost_type(cost_type) {
}

class CostAdaptedHeuristicFeature
    : public plugins::TaskIndependentFeature<TaskIndependentEvaluator> {
public:
    CostAdaptedHeuristicFeature() : TaskIndependentFeature("cost_adapted_heuristic") {
        document_title("Cost-adapted heuristic");
        document_synopsis(
            "Evaluates the nested heuristic on a task with a modified cost"
            "function");

        add_option<shared_ptr<TaskIndependentEvaluator>>(
            "nested",
            "the heuristic that should be computed under modified costs");
        add_option<OperatorCost>(
            "cost_type",
            "Operator cost adjustment type. No matter what this setting is, "
            "axioms will always be considered as actions of cost 0 by the "
            "heuristics that treat axioms as actions.",
            "normal");

        document_language_support(
            "action costs", "supported if the nested heuristic does");
        document_language_support(
            "conditional effects", "supported if the nested heuristic does");
        document_language_support(
            "axioms", "supported if the nested heuristic does");

        document_property("admissible", "if the nested heuristic is");
        document_property("consistent", "if the nested heuristic is");
        document_property("safe", "if the nested heuristic is");
        document_property(
            "preferred operators", "if the nested heuristic supports them");
    }

    virtual shared_ptr<TaskIndependentEvaluator> create_component(
        const plugins::Options &opts) const override {
        return make_shared<TaskIndependentCostAdaptedHeuristic>(
            opts.get<shared_ptr<TaskIndependentEvaluator>>("nested"),
            opts.get<OperatorCost>("cost_type"));
    }
};

static plugins::FeaturePlugin<CostAdaptedHeuristicFeature> _plugin;
}
