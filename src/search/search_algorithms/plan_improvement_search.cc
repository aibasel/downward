
#include "plan_improvement_search.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <memory>
#include <string>

using namespace std;

namespace plan_improvement_search {

PlanImprovementSearch::PlanImprovementSearch(
    const shared_ptr<AbstractTask> &task,
    const shared_ptr<TaskIndependentSearchAlgorithm> &search_config,
    OperatorCost cost_type, int bound, double max_time,
    const shared_ptr<PlanImprovement> &improvement, const string &description,
    utils::Verbosity verbosity)
    : SearchAlgorithm(task, cost_type, bound, max_time, description, verbosity),
      search_config(search_config),
      improvement(improvement) {
}

SearchStatus PlanImprovementSearch::step() {
    // Create the task-specific search algorithm.
    log << "Before we bind the task to the search" << endl;
    shared_ptr<SearchAlgorithm> inner_search = search_config->bind_task(task);

    log << "Starting search: " << inner_search->get_description() << endl;

    inner_search->search();

    if (inner_search->found_solution()) {
        Plan plan = inner_search->get_plan();

        if (improvement) {
            plan = improvement->improve(plan, task_proxy, state_registry);
        }

        set_plan(plan);
    }

    const SearchStatistics &inner_stats = inner_search->get_statistics();

    statistics.inc_expanded(inner_stats.get_expanded());
    statistics.inc_evaluated_states(inner_stats.get_evaluated_states());
    statistics.inc_evaluations(inner_stats.get_evaluations());
    statistics.inc_generated(inner_stats.get_generated());
    statistics.inc_generated_ops(inner_stats.get_generated_ops());
    statistics.inc_reopened(inner_stats.get_reopened());

    inner_complete_within_bound = inner_search->is_complete_within_bound() &&
                                  inner_search->get_bound() >= bound;

    SearchStatus inner_status = inner_search->get_status();

    if (inner_search->found_solution()) {
        return SOLVED;
    }

    if (!inner_complete_within_bound &&
        (inner_status == UNSOLVABLE_WITHIN_BOUND ||
         inner_status == UNSOLVABLE)) {
        return FAILED;
    }

    return inner_status;
}

void PlanImprovementSearch::print_statistics() const {
    log << "Search statistics:" << endl;
    statistics.print_detailed_statistics();
}

bool PlanImprovementSearch::is_complete_within_bound() const {
    return inner_complete_within_bound;
}

class TaskIndependentPlanImprovementSearch
    : public components::TaskIndependentComponent<SearchAlgorithm> {
    shared_ptr<TaskIndependentSearchAlgorithm> search_config;
    OperatorCost cost_type;
    int bound;
    double max_time;
    shared_ptr<PlanImprovement> improvement;
    string description;
    utils::Verbosity verbosity;

protected:
    shared_ptr<SearchAlgorithm> create_task_specific_component(
        const shared_ptr<AbstractTask> &task) const override {
        return make_shared<PlanImprovementSearch>(
            task, search_config, cost_type, bound, max_time, improvement,
            description, verbosity);
    }

public:
    TaskIndependentPlanImprovementSearch(
        const shared_ptr<TaskIndependentSearchAlgorithm> &search_config,
        const shared_ptr<PlanImprovement> &improvement, OperatorCost cost_type,
        int bound, double max_time, const string &description,
        utils::Verbosity verbosity)
        : search_config(search_config),
          cost_type(cost_type),
          bound(bound),
          max_time(max_time),
          improvement(improvement),
          description(description),
          verbosity(verbosity) {
    }
};

class PlanImprovementSearchFeature
    : public plugins::TypedFeature<TaskIndependentSearchAlgorithm> {

public:
    PlanImprovementSearchFeature() : TypedFeature("plan_improvement") {
        document_title("Plan improvement search");

        document_synopsis(
            "Runs another search algorithm and provides "
            "a framework for post-processing its solution.");

        add_option<shared_ptr<TaskIndependentSearchAlgorithm>>(
            "search", "search algorithm to execute", "");

        add_option<shared_ptr<PlanImprovement>>(
            "method", "plan improvement method to apply", "");

        add_search_algorithm_options_to_feature(*this, "plan_improvement");
    }

    shared_ptr<TaskIndependentSearchAlgorithm> create_component(
        const plugins::Options &opts) const override {
        return components::make_shared_from_arg_tuples<
            TaskIndependentPlanImprovementSearch>(
            opts.get<shared_ptr<TaskIndependentSearchAlgorithm>>("search"),
            opts.get<shared_ptr<PlanImprovement>>("method"),
            get_search_algorithm_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<PlanImprovementSearchFeature> _plugin;
}
