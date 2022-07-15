#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H

#include "landmark_factory_relaxation.h"

namespace landmarks {
class LandmarkFactoryRpgExhaust : public LandmarkFactoryRelaxation {
    const bool only_causal_landmarks;
    virtual void generate_relaxed_landmarks(const std::shared_ptr<AbstractTask> &task,
                                            Exploration &exploration) override;

public:
    explicit LandmarkFactoryRpgExhaust(const options::Options &opts);

    virtual bool computes_reasonable_orders() const override;
    virtual bool supports_conditional_effects() const override;
};
}

#endif
