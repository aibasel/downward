#ifndef LANDMARKS_LAMA_FF_SYNERGY_H
#define LANDMARKS_LAMA_FF_SYNERGY_H

#include "../heuristic.h"

#include <memory>

class Exploration;
class LandmarkCountHeuristic;

class FFSlaveHeuristic;
class LamaMasterHeuristic;

class LamaFFSynergy {
    friend class LamaMasterHeuristic;
    friend class FFSlaveHeuristic;

    std::unique_ptr<LamaMasterHeuristic> lama_master_heuristic;
    std::unique_ptr<FFSlaveHeuristic> ff_slave_heuristic;
    const bool lm_pref;
    const bool lm_admissible;
    const bool lm_optimal;
    const bool use_action_landmarks;

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
