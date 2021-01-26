#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H

#include "landmark_factory_relaxation.h"

namespace landmarks {
class LandmarkFactoryRpgExhaust : public LandmarkFactoryRelaxation {
    virtual void generate_relaxed_landmarks(const std::shared_ptr<AbstractTask> &task,
                                            Exploration &exploration) override;

public:
    explicit LandmarkFactoryRpgExhaust(const options::Options &opts);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
