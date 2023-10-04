#ifndef HEURISTICS_LM_CUT_HEURISTIC_H
#define HEURISTICS_LM_CUT_HEURISTIC_H

#include "../heuristic.h"

#include <memory>

namespace plugins {
class Options;
}

namespace lm_cut_heuristic {
class LandmarkCutLandmarks;

class LandmarkCutHeuristic : public Heuristic {
    std::unique_ptr<LandmarkCutLandmarks> landmark_generator;

    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit LandmarkCutHeuristic(std::basic_string<char> unparsed_config,
                                  utils::LogProxy log,
                                  bool cache_evaluator_values,
                                  std::shared_ptr<AbstractTask> task);
    virtual ~LandmarkCutHeuristic() override;
};

class TaskIndependentLandmarkCutHeuristic : public TaskIndependentHeuristic {
private:
    std::string unparsed_config;
    utils::LogProxy log;
    bool cache_evaluator_values;
public:
    explicit TaskIndependentLandmarkCutHeuristic(std::string unparsed_config,
                                                 utils::LogProxy log,
                                                 bool cache_evaluator_values,
                                                 std::shared_ptr<TaskIndependentAbstractTask> task_transformation
                                                 );

    virtual ~TaskIndependentLandmarkCutHeuristic()  override;

    virtual std::shared_ptr<Heuristic> create_task_specific_Heuristic(std::shared_ptr<AbstractTask> &task, int depth = -1) override;

    virtual std::shared_ptr<Heuristic> create_task_specific_Heuristic(
        std::shared_ptr<AbstractTask> &task,
        std::shared_ptr<ComponentMap> &component_map, int depth = -1) override;

    virtual std::shared_ptr<LandmarkCutHeuristic> create_task_specific_LandmarkCutHeuristic(std::shared_ptr<AbstractTask> &task, int depth = -1);
    virtual std::shared_ptr<LandmarkCutHeuristic> create_task_specific_LandmarkCutHeuristic(std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map, int depth = -1);
};
}

#endif
