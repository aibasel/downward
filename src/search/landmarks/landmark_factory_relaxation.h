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
    bool relaxed_task_solvable(
        const TaskProxy &task_proxy, Exploration &exploration,
        const Landmark &exclude, bool use_unary_relaxation=false) const;
    /*
      Compute for each fact whether it is relaxed reachable without
      achieving the excluded landmark.
    */
    std::vector<std::vector<bool>> compute_relaxed_reachability(
        Exploration &exploration, const Landmark &exclude,
        bool use_unary_relaxation=false) const;

private:
    void generate_landmarks(const std::shared_ptr<AbstractTask> &task) override;

    virtual void generate_relaxed_landmarks(const std::shared_ptr<AbstractTask> &task,
                                            Exploration &exploration) = 0;
    void postprocess(const TaskProxy &task_proxy, Exploration &exploration);

    void calc_achievers(const TaskProxy &task_proxy, Exploration &exploration);
};
}

#endif
