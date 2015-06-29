#ifndef LANDMARKS_LAMA_FF_SYNERGY_H
#define LANDMARKS_LAMA_FF_SYNERGY_H

#include "../heuristic.h"

#include <memory>

class Exploration;
class LandmarkCountHeuristic;

class FFSlaveHeuristic;
class LamaMasterHeuristic;

/*
  TODO: The synergy class stores EvaluationResult objects for "the
  current state", a concept we want to move away with.

  A different and perhaps better implementation would get rid of the
  lama_result and ff_result members of LamaFFSynergy and instead only
  use the EvaluationContext. But this would require either extending
  the interface of EvaluationContext or perform some more significant
  restructuring of the landmark code. We have wanted to do the latter
  for a while, so it perhaps make sense to defer a change of the
  synergy implementation until this point.
*/

class LamaFFSynergy {
    friend class LamaMasterHeuristic;
    friend class FFSlaveHeuristic;

    std::unique_ptr<LamaMasterHeuristic> lama_master_heuristic;
    std::unique_ptr<FFSlaveHeuristic> ff_slave_heuristic;

    std::unique_ptr<LandmarkCountHeuristic> lama_heuristic;
    Exploration *exploration;

    EvaluationResult lama_result;
    EvaluationResult ff_result;

    bool lama_reach_state(
        const GlobalState &parent_state, const GlobalOperator &op,
        const GlobalState &state);

    void compute_heuristics(EvaluationContext &eval_context);

public:
    explicit LamaFFSynergy(const Options &opts);
    ~LamaFFSynergy() = default;

    Heuristic *get_lama_heuristic_proxy() const;
    Heuristic *get_ff_heuristic_proxy() const;
};

#endif
