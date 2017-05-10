#ifndef LANDMARKS_LAMA_SYNERGY_H
#define LANDMARKS_LAMA_SYNERGY_H

#include "ff_synergy.h"
#include "landmark_count_heuristic.h"

#include "../heuristic.h"

namespace landmarks {
class FFSynergyHeuristic;
class LandmarkCountHeuristic;
class LandmarkFactory;


class LamaSynergyHeuristic : public Heuristic {
    friend FFSynergyHeuristic;

    std::unique_ptr<LandmarkCountHeuristic> lama_heuristic;

    EvaluationResult ff_result;
    EvaluationResult lama_result;

    void compute_heuristics(EvaluationContext &eval_context);

protected:
    virtual int compute_heuristic(const GlobalState & /*state*/) override {
        ABORT("This method should never be called.");
    }

public:
    explicit LamaSynergyHeuristic(const options::Options &opts);

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void notify_initial_state(const GlobalState &initial_state) override;

    virtual bool notify_state_transition(
        const GlobalState &parent_state,
        const GlobalOperator &op,
        const GlobalState &state) override;
};
}

#endif
