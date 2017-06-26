#ifndef LANDMARKS_FF_SYNERGY_H
#define LANDMARKS_FF_SYNERGY_H

#include "../heuristic.h"

namespace landmarks {
class LamaSynergyHeuristic;

// See documentation of LamaSynergyHeuristic.
class FFSynergyHeuristic : public Heuristic {
    LamaSynergyHeuristic *master;

protected:
    virtual int compute_heuristic(const GlobalState & /*state*/) override {
        ABORT("This method should never be called.");
    }

public:
    explicit FFSynergyHeuristic(const options::Options &opts);

    virtual EvaluationResult compute_result(EvaluationContext &eval_context) override;
};
}
#endif
