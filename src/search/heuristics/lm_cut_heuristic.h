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
                                  utils::Verbosity verbosity,
                                  const std::shared_ptr<AbstractTask> task,
                                  bool cache_evaluator_values);
    virtual ~LandmarkCutHeuristic() override;
};

class TaskIndependentLandmarkCutHeuristic : public TaskIndependentHeuristic {
private:
    bool cache_evaluator_values;
protected:
    std::string get_product_name() const override { return "LandmarkCutHeuristic";}
public:
    explicit TaskIndependentLandmarkCutHeuristic(const std::string &name,
                                                 utils::Verbosity verbosity,
                                                 std::shared_ptr<TaskIndependentAbstractTask> task_transformation,
                                                 bool cache_evaluator_values);

    virtual ~TaskIndependentLandmarkCutHeuristic()  override;

    using AbstractProduct = Evaluator;
    using ConcreteProduct = LandmarkCutHeuristic;

    std::shared_ptr<AbstractProduct>
    get_task_specific(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map,
                      int depth = -1) const override;

    std::shared_ptr<ConcreteProduct> create_ts(
            const std::shared_ptr<AbstractTask> &task,
            std::unique_ptr<ComponentMap> &component_map,
            int depth) const;
};
}

#endif
