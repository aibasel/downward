#ifndef HEURISTICS_LM_CUT_HEURISTIC_H
#define HEURISTICS_LM_CUT_HEURISTIC_H

#include "lm_cut_landmarks.h"

#include "../heuristic.h"

#include <memory>

class GlobalState;

namespace options {
class Options;
}

namespace lm_cut_heuristic {
class LandmarkCutHeuristic : public Heuristic {
    std::unique_ptr<LandmarkCutLandmarks> landmark_generator;

    virtual void initialize() override;
    virtual int compute_heuristic(const GlobalState &global_state) override;
    int compute_heuristic(const State &state);
public:
    explicit LandmarkCutHeuristic(const options::Options &opts);
    virtual ~LandmarkCutHeuristic() override;
};
}

#endif
