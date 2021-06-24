#ifndef LANDMARKS_LANDMARK_FACTORY_MERGED_H
#define LANDMARKS_LANDMARK_FACTORY_MERGED_H

#include "landmark_factory.h"

#include <vector>

namespace landmarks {
class LandmarkFactoryMerged : public LandmarkFactory {
    std::vector<std::shared_ptr<LandmarkFactory>> lm_factories;

    virtual void generate_landmarks(const std::shared_ptr<AbstractTask> &task) override;
    void postprocess();
    LandmarkNode *get_matching_landmark(const Landmark &landmark) const;
public:
    explicit LandmarkFactoryMerged(const options::Options &opts);

    virtual bool computes_reasonable_orders() const override;
    virtual bool supports_conditional_effects() const override;
};
}

#endif
