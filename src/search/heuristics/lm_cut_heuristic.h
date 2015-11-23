#ifndef LM_CUT_HEURISTIC_H
#define LM_CUT_HEURISTIC_H

#include "lm_cut_landmarks.h"

#include "../heuristic.h"

#include <memory>

class GlobalState;
class Options;

namespace LMcutHeuristic {
class LandmarkCutHeuristic : public Heuristic {
    std::unique_ptr<LandmarkCutLandmarks> landmark_generator;

    virtual void initialize() override;
    virtual int compute_heuristic(const GlobalState &global_state) override;
    int compute_heuristic(const State &state);
public:
    explicit LandmarkCutHeuristic(const Options &opts);
    virtual ~LandmarkCutHeuristic() override;
};
}

#endif
