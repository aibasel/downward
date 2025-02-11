#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H

#include "landmark_factory_relaxation.h"

namespace landmarks {
class LandmarkFactoryRpgExhaust : public LandmarkFactoryRelaxation {
    const bool only_causal_landmarks;
    virtual void generate_relaxed_landmarks(const std::shared_ptr<AbstractTask> &task,
                                            Exploration &exploration) override;

public:
    explicit LandmarkFactoryRpgExhaust(bool only_causal_landmarks,
                                       utils::Verbosity verbosity);

    virtual bool supports_conditional_effects() const override;
};

extern void add_only_causal_landmarks_option_to_feature(plugins::Feature &feature);
extern bool get_only_causal_landmarks_arguments_from_options(
    const plugins::Options &opts);
}

#endif
