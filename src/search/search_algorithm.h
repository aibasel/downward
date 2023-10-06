#ifndef SEARCH_ALGORITHM_H
#define SEARCH_ALGORITHM_H

#include "abstract_task.h"
#include "component_map.h"
#include "operator_cost.h"
#include "operator_id.h"
#include "plan_manager.h"
#include "search_progress.h"
#include "search_space.h"
#include "search_statistics.h"
#include "state_registry.h"
#include "task_proxy.h"

#include "utils/logging.h"

#include <vector>

namespace plugins {
class Feature;
}

namespace ordered_set {
template<typename T>
class OrderedSet;
}

namespace successor_generator {
class SuccessorGenerator;
}

enum SearchStatus {IN_PROGRESS, TIMEOUT, FAILED, SOLVED};

class SearchAlgorithm : public Component {
    std::string description;
    SearchStatus status;
    bool solution_found;
    Plan plan;
protected:
    // Hold a reference to the task implementation and pass it to objects that need it.
    const std::shared_ptr<AbstractTask> task;
    // Use task_proxy to access task information.
    TaskProxy task_proxy;

    mutable utils::LogProxy log;
    PlanManager plan_manager;
    StateRegistry state_registry;
    const successor_generator::SuccessorGenerator &successor_generator;
    SearchSpace search_space;
    SearchProgress search_progress;
    SearchStatistics statistics;
    int bound;
    OperatorCost cost_type;
    bool is_unit_cost;
    double max_time;

    virtual void initialize() {}
    virtual SearchStatus step() = 0;

    void set_plan(const Plan &plan);
    bool check_goal_and_set_plan(const State &state);
    int get_adjusted_cost(const OperatorProxy &op) const;
public:
    SearchAlgorithm(utils::Verbosity verbosity,
                 OperatorCost cost_type,
                 double max_time,
                 int bound,
                 std::string unparsed_config,
                 const std::shared_ptr<AbstractTask> task);
    virtual ~SearchAlgorithm();
    virtual void print_statistics() const = 0;
    virtual void save_plan_if_necessary();
    bool found_solution() const;
    SearchStatus get_status() const;
    const Plan &get_plan() const;
    void search();
    const SearchStatistics &get_statistics() const {return statistics;}
    void set_bound(int b) {bound = b;}
    int get_bound() {return bound;}
    PlanManager &get_plan_manager() {return plan_manager;}
    std::string get_description() {return description;}

    /* The following three methods should become functions as they
       do not require access to private/protected class members. */
    static void add_pruning_option(plugins::Feature &feature);
    static void add_options_to_feature(plugins::Feature &feature);
    static void add_succ_order_options(plugins::Feature &feature);
};

class TaskIndependentSearchAlgorithm : public TaskIndependentComponent {
    std::string description;
    SearchStatus status;
    bool solution_found;
    Plan plan;
protected:

    mutable utils::Verbosity verbosity;
    PlanManager plan_manager;
    SearchProgress search_progress;
    int bound;
    OperatorCost cost_type;
    bool is_unit_cost;
    double max_time;

public:
    TaskIndependentSearchAlgorithm(utils::Verbosity verbosity,
                                OperatorCost cost_type,
                                double max_time,
                                int bound,
                                std::string unparsed_config);
    virtual ~TaskIndependentSearchAlgorithm();

    PlanManager &get_plan_manager() {return plan_manager;}
    std::string get_description() {return description;}

    virtual std::shared_ptr<SearchAlgorithm> create_task_specific_root(const std::shared_ptr<AbstractTask> &task, int depth = -1);

    virtual std::shared_ptr<SearchAlgorithm>
    create_task_specific(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map,
                         int depth = -1) = 0;
};

/*
  Print evaluator values of all evaluators evaluated in the evaluation context.
*/
extern void print_initial_evaluator_values(
    const EvaluationContext &eval_context);

extern void collect_preferred_operators(
    EvaluationContext &eval_context, Evaluator *preferred_operator_evaluator,
    ordered_set::OrderedSet<OperatorID> &preferred_operators);

#endif
