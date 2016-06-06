#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H

#include "landmark_factory.h"

namespace landmarks {
class LandmarkFactoryRpgExhaust : public LandmarkFactory {
public:
    explicit LandmarkFactoryRpgExhaust(const options::Options &opts);
    virtual ~LandmarkFactoryRpgExhaust() override = default;

    virtual bool supports_conditional_effects() const override;
private:
    virtual void generate_landmarks(Exploration &exploration) override;
};
}

#endif
