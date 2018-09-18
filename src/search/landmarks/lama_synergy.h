#ifndef LANDMARKS_LAMA_SYNERGY_H
#define LANDMARKS_LAMA_SYNERGY_H

#include "../evaluation_result.h"
#include "../heuristic.h"

namespace landmarks {
class FFSynergyHeuristic;
class LandmarkCountHeuristic;
class LandmarkFactory;

/*
  The LamaSynergyHeuristic allows exploiting synergy effects when using
  both the FF heuristic (for its estimates or its preferred operators)
  and preferred operators of the landmark count (lama) heuristic.

  This synergy can be used via predefinitions (see user documentation in
  lama_synergy.cc).

  The interaction of LamaSynergyHeuristic and FFSynergyHeuristic works as
  follows: the lama synergy acts as the master heuristic, wrapping the actual
  lama (landmark count) heuristic. When computing the heuristic value for the
  lama heuristic, either the heuristic values of both the lama synergy and the
  ff synergy must be computed, or both have been previously computed and are
  cached in the lama synergy. When computing the heuristic value for the ff
  synergy, the ff synergy triggers the computation or lookup of its master lama
  synergy.

  Note that both synergy classes forbid to call compute_heuristic but overwrite
  the public method compute_result for the computation of heuristic values.
*/
class LamaSynergyHeuristic : public Heuristic {
    friend FFSynergyHeuristic;

    const std::unique_ptr<LandmarkCountHeuristic> lama_heuristic;

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

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override {
        evals.insert(this);
    }

    virtual void notify_initial_state(const GlobalState &initial_state) override;

    virtual void notify_state_transition(
        const GlobalState &parent_state,
        OperatorID op_id,
        const GlobalState &state) override;
};
}

#endif
