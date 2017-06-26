#ifndef LANDMARKS_LAMA_SYNERGY_H
#define LANDMARKS_LAMA_SYNERGY_H

#include "../evaluation_result.h"
#include "../heuristic.h"

namespace landmarks {
class FFSynergyHeuristic;
class LandmarkCountHeuristic;
class LandmarkFactory;

/*
  This class can be used if the FF heuristic should be used (for its estimates
  or its preferred operators) and we want to use preferred operators of the
  landmark count heuristic, we can exploit synergy effects by using the LAMA-FF
  synergy. This synergy can be used via Predefinition (see
  OptionSyntax#Predefinitions), for example:
    --heuristic lama_master=lama_synergy(...)
    --heuristic lama_slave=ff_synergy(lama_master)

  The interaction of LamaSynergyHeuristic and FFSynergyHeuristic works as
  follows: the lama synergy acts as the master heuristic, wrapping the actual
  lama (landmark count) heuristic. If computing the heuristic value for the
  lama heuristic, either the heuristic values of both the lama synergy and the
  ff synergy must be computed, or both have been previously computed and are
  cached in the lama synergy. If computing the heuristic value for the ff
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

    virtual void notify_initial_state(const GlobalState &initial_state) override;

    virtual bool notify_state_transition(
        const GlobalState &parent_state,
        const GlobalOperator &op,
        const GlobalState &state) override;
};
}

#endif
