#ifndef LANDMARKS_LANDMARK_FACTORY_RELAXATION_H
#define LANDMARKS_LANDMARK_FACTORY_RELAXATION_H

#include "landmark_factory.h"

namespace landmarks {
class Exploration;

class LandmarkFactoryRelaxation : public LandmarkFactory {
protected:
    explicit LandmarkFactoryRelaxation(utils::Verbosity verbosity);

    /*
      Test whether the relaxed planning task is solvable without
      achieving the excluded landmark.
    */
    bool relaxed_task_solvable(const TaskProxy &task_proxy,
                               Exploration &exploration,
                               const Landmark &exclude) const;
    /*
      Compute for each fact whether it is relaxed reachable without
      achieving the excluded landmark.
    */
    std::vector<std::vector<bool>> compute_relaxed_reachability(
        Exploration &exploration, const Landmark &exclude) const;

private:
    void generate_landmarks(const std::shared_ptr<AbstractTask> &task) override;

    virtual void generate_relaxed_landmarks(const std::shared_ptr<AbstractTask> &task,
                                            Exploration &exploration) = 0;
    void postprocess(const TaskProxy &task_proxy, Exploration &exploration);

    void calc_achievers(const TaskProxy &task_proxy, Exploration &exploration);

protected:
    /*
      The method discard_noncausal_landmarks assumes the graph has no
      conjunctive landmarks, and will not process conjunctive landmarks
      correctly.
    */
    void discard_noncausal_landmarks(const TaskProxy &task_proxy,
                                     Exploration &exploration);
    /*
      A landmark is causal if it is a goal or it is a precondition of an
      action that must be applied in order to reach the goal.
    */
    bool is_causal_landmark(const TaskProxy &task_proxy,
                            Exploration &exploration,
                            const Landmark &landmark) const;
};
}

#endif
