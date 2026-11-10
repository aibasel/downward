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
    inner_search = search_config->bind_task(task);

    if (bound < inner_search->get_bound()) {
        inner_search->set_bound(bound);
    }

    inner_search->search();

    SearchStatus inner_status = inner_search->get_finished_search_status();
    bool found_inner_solution = inner_search->found_solution();

    Plan plan;
    if (found_inner_solution) {
        plan = inner_search->get_plan();
    }

    int inner_bound = inner_search->get_bound();
    inner_complete_within_bound =
        inner_search->is_complete_within_bound() && inner_bound >= bound;
    const SearchStatistics &inner_stats = inner_search->get_statistics();
    statistics.inc_expanded(inner_stats.get_expanded());
    statistics.inc_evaluated_states(inner_stats.get_evaluated_states());
    statistics.inc_evaluations(inner_stats.get_evaluations());
    statistics.inc_generated(inner_stats.get_generated());
    statistics.inc_generated_ops(inner_stats.get_generated_ops());
    statistics.inc_reopened(inner_stats.get_reopened());

    if (found_inner_solution) {
        initial_plan_length = plan.size();
        initial_plan_cost = calculate_plan_cost(plan, task_proxy);
        set_plan(plan);

        improvement_timer.resume();

        Plan improved_plan =
            improvement->improve(plan, task_proxy, state_registry);

        improvement_timer.stop();

        final_plan_length = improved_plan.size();
        final_plan_cost = calculate_plan_cost(improved_plan, task_proxy);

        set_plan(improved_plan);
    }
    if (inner_status == SOLVED) {
        return SOLVED;
    }
    if (inner_status == UNSOLVABLE_WITHIN_BOUND && inner_bound < bound) {
        return FAILED;
    }
    return inner_status;
}

void PlanImprovementSearch::print_statistics() const {
    log << "Cumulative statistics:" << endl;
    statistics.print_detailed_statistics();
    if (found_solution()) {
        log << "Plan improvement statistics: " << endl;
        log << "Initial plan length: " << initial_plan_length << endl;
        log << "Final plan length: " << final_plan_length << endl;
        log << "Initial plan cost: " << initial_plan_cost << endl;
        log << "Final plan cost: " << final_plan_cost << endl;
        log << "Plan improvement time: " << improvement_timer << endl;
        improvement->print_statistics();
    }
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

    void get_task_preserving_subcomponents(
        vector<components::TaskIndependentComponentBase *> &components)
        const override {
        components.push_back(search_config.get());
        search_config->get_task_preserving_subcomponents(components);
    }
};

class PlanImprovementSearchFeature
    : public plugins::TypedFeature<TaskIndependentSearchAlgorithm> {
public:
    PlanImprovementSearchFeature() : TypedFeature("plan_improvement") {
        document_title("Plan improvement search");
        document_synopsis(
            "Runs a specified search algorithm to find an initial plan and"
            "applies a plan improvement method to the resulting solution");
        add_option<shared_ptr<TaskIndependentSearchAlgorithm>>(
            "search", "Search algorithm to find the initial plan", "");
        add_option<shared_ptr<PlanImprovement>>(
            "improvement_method",
            "Post-processing plan improvement method applied to the initial plan",
            "");
        add_search_algorithm_options_to_feature(*this, "plan_improvement");
        document_note(
            "Usage",
            "The configuration\n```\n"
            "--search \"plan_improvement("
            "search=lazy_greedy([ff()]),"
            "improvement_method=action_elimination())\"\n"
            "```\nwould run lazy_greedy search with the ff heuristic to find an initial plan, then try to improve that plan with action elimintion\n\n"
            "```");
    }
    shared_ptr<TaskIndependentSearchAlgorithm> create_component(
        const plugins::Options &opts) const override {
        return components::make_shared_from_arg_tuples<
            TaskIndependentPlanImprovementSearch>(
            opts.get<shared_ptr<TaskIndependentSearchAlgorithm>>("search"),
            opts.get<shared_ptr<PlanImprovement>>("improvement_method"),
            get_search_algorithm_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<PlanImprovementSearchFeature> _plugin;
}