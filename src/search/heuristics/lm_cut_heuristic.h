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
    explicit LandmarkCutHeuristic(std::string name,
                                  std::basic_string<char> unparsed_config,
                                  utils::LogProxy log,
                                  bool cache_evaluator_values,
                                  const std::shared_ptr<AbstractTask> task);
    virtual ~LandmarkCutHeuristic() override;
};

class TaskIndependentLandmarkCutHeuristic : public TaskIndependentHeuristic {
private:
    std::string name;
    std::string unparsed_config;
    utils::LogProxy log;
    bool cache_evaluator_values;
public:
    explicit TaskIndependentLandmarkCutHeuristic(std::string name,
                                                 std::string unparsed_config,
                                                 utils::LogProxy log,
                                                 bool cache_evaluator_values,
                                                 std::shared_ptr<TaskIndependentAbstractTask> task_transformation);

    virtual ~TaskIndependentLandmarkCutHeuristic()  override;

    std::shared_ptr<Evaluator>
    create_task_specific(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map,
                         int depth = -1 ) override;
};
}

#endif
