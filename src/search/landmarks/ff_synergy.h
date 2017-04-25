#ifndef LANDMARKS_FF_SYNERGY_H
#define LANDMARKS_FF_SYNERGY_H

#include "../heuristic.h"

#include "lama_synergy.h"

namespace landmarks{
class LamaSynergyHeuristic;

class FFSynergyHeuristic : public Heuristic {
    LamaSynergyHeuristic *master;

protected:
    virtual int compute_heuristic(const GlobalState & /*state*/) override {
        ABORT("This method should never be called.");
    }

public:
    explicit FFSynergyHeuristic(const options::Options &opts);

    virtual EvaluationResult compute_result(EvaluationContext &eval_context) override;

    virtual ~FFSynergyHeuristic() override = default;
};

}
#endif
