#ifndef HEURISTICS_LM_CUT_HEURISTIC_H
#define HEURISTICS_LM_CUT_HEURISTIC_H

#include "../heuristic.h"

#include <memory>


namespace lm_cut_heuristic {
class LandmarkCutLandmarks;

class LandmarkCutHeuristic : public Heuristic {
    std::unique_ptr<LandmarkCutLandmarks> landmark_generator;

    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit LandmarkCutHeuristic(const std::string &name,
                                  std::string unparsed_config,
                                  utils::Verbosity verbosity,
                                  const std::shared_ptr<AbstractTask> task,
                                  bool cache_evaluator_values);
    virtual ~LandmarkCutHeuristic() override;
};

class TaskIndependentLandmarkCutHeuristic : public TaskIndependentHeuristic {
private:
    std::string unparsed_config;
    bool cache_evaluator_values;
public:
    explicit TaskIndependentLandmarkCutHeuristic(const std::string &name,
                                                 std::string unparsed_config,
                                                 utils::Verbosity verbosity,
                                                 std::shared_ptr<TaskIndependentAbstractTask> task_transformation,
                                                 bool cache_evaluator_values);

    virtual ~TaskIndependentLandmarkCutHeuristic()  override;

    std::shared_ptr<Evaluator>
    create_task_specific(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map,
                         int depth = -1) const override;
};
}

#endif
