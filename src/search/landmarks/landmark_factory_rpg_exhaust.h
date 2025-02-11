#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H

#include "landmark_factory_relaxation.h"

namespace landmarks {
class LandmarkFactoryRpgExhaust : public LandmarkFactoryRelaxation {
    const bool use_unary_relaxation;
    virtual void generate_relaxed_landmarks(const std::shared_ptr<AbstractTask> &task,
                                            Exploration &exploration) override;

public:
    explicit LandmarkFactoryRpgExhaust(bool use_unary_relaxation,
                                       utils::Verbosity verbosity);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
