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
    LandmarkCutHeuristic(
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates, const std::string &description,
        utils::Verbosity verbosity);
};

class TaskIndependentLandmarkCutHeuristic : public TaskIndependentHeuristic {
private:
    bool cache_evaluator_values;
public:
    TaskIndependentLandmarkCutHeuristic(
        const std::shared_ptr<TaskIndependentAbstractTask> transform,
        bool cache_estimates,
        const std::string &description,
        utils::Verbosity verbosity);

    virtual ~TaskIndependentLandmarkCutHeuristic()  override;

    std::shared_ptr<Evaluator> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const override;
};
}

#endif
