#ifndef DOWNWARD_TASK_INDEPENDENT_SEARCH_ENGINE_H
#define DOWNWARD_TASK_INDEPENDENT_SEARCH_ENGINE_H

#include "abstract_task.h"

#include "operator_cost.h"
#include "plan_manager.h"

class TaskIndependentSearchEngine{
    std::string description;
    SearchStatus status;
    bool solution_found;
    Plan plan;
protected:

    mutable utils::Verbosity verbosity;
    PlanManager plan_manager;
    const successor_generator::SuccessorGenerator &successor_generator;
    SearchSpace search_space;
    SearchProgress search_progress;
    int bound;
    OperatorCost cost_type;
    bool is_unit_cost;
    double max_time;

public:
    //TaskIndependentSearchEngine(const plugins::Options &opts);
    TaskIndependentSearchEngine(utils::Verbosity verbosity,
    OperatorCost cost_type,
    double max_time,
    int bound,
            std::string unparsed_config);
    virtual ~TaskIndependentSearchEngine();

    PlanManager &get_plan_manager() {return plan_manager;}


    std::shared_ptr<SearchEngine> create_task_specific(std::shared_ptr<AbstractTask> &task);

};


#endif //DOWNWARD_TASK_INDEPENDENT_SEARCH_ENGINE_H
