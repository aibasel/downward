#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H

#include "landmark_factory_relaxation.h"

namespace landmarks {
class LandmarkFactoryRpgExhaust : public LandmarkFactoryRelaxation {
    const bool use_unary_relaxation;
    void generate_goal_landmarks(const TaskProxy &task_proxy) const;
    void generate_all_atomic_landmarks(
        const TaskProxy &task_proxy, Exploration &exploration) const;
    virtual void generate_relaxed_landmarks(
        const std::shared_ptr<AbstractTask> &task,
        Exploration &exploration) override;

public:
    LandmarkFactoryRpgExhaust(
        const std::shared_ptr<AbstractTask> &task, bool use_unary_relaxation,
        utils::Verbosity verbosity);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
