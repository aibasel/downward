#ifndef LANDMARKS_LANDMARK_FACTORY_RELAXATION_H
#define LANDMARKS_LANDMARK_FACTORY_RELAXATION_H

#include "landmark_factory.h"

namespace landmarks {
class LandmarkFactoryRelaxation : public LandmarkFactory {
protected:
    explicit LandmarkFactoryRelaxation(const options::Options &opts)
        : LandmarkFactory(opts) {}

private:
    void generate_landmarks(const std::shared_ptr<AbstractTask> &task) override;

    virtual void generate_landmarks(const std::shared_ptr<AbstractTask> &task, Exploration &exploration) = 0;
};
}

#endif
