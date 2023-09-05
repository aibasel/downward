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
                                                 bool cache_evaluator_values);
    virtual std::shared_ptr<Evaluator> create_task_specific(std::shared_ptr<AbstractTask> &task) override;

    virtual ~TaskIndependentLandmarkCutHeuristic()  override;
};
}

#endif
