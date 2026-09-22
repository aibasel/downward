#ifndef LANDMARKS_LANDMARK_FACTORY_RELAXATION_H
#define LANDMARKS_LANDMARK_FACTORY_RELAXATION_H

#include "landmark_factory.h"

namespace landmarks {
class Exploration;

class LandmarkFactoryRelaxation : public LandmarkFactory {
    void generate_landmarks(const std::shared_ptr<AbstractTask> &task) override;

    virtual void generate_relaxed_landmarks(
        const std::shared_ptr<AbstractTask> &task,
        Exploration &exploration) = 0;
    void postprocess(const TaskProxy &task_proxy, Exploration &exploration);

    void compute_possible_achievers(
        Landmark &landmark, const VariablesProxy &variables);
    void calc_achievers(const TaskProxy &task_proxy, Exploration &exploration);

protected:
    LandmarkFactoryRelaxation(
        const std::shared_ptr<AbstractTask> &task, utils::Verbosity verbosity);
};
}

#endif
