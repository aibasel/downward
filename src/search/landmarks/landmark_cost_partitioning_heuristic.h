#ifndef LANDMARKS_LANDMARK_COST_PARTITIONING_HEURISTIC_H
#define LANDMARKS_LANDMARK_COST_PARTITIONING_HEURISTIC_H

#include "landmark_heuristic.h"

class BitsetView;

namespace successor_generator {
class SuccessorGenerator;
}

namespace landmarks {
class LandmarkCostAssignment;
class LandmarkGraph;
class LandmarkNode;
class LandmarkStatusManager;

class LandmarkCostPartitioningHeuristic : public LandmarkHeuristic {
    std::shared_ptr<LandmarkGraph> lgraph;
    const bool use_preferred_operators;
    const bool conditional_effects_supported;

    std::unique_ptr<LandmarkStatusManager> lm_status_manager;
    std::unique_ptr<LandmarkCostAssignment> lm_cost_assignment;
    std::unique_ptr<successor_generator::SuccessorGenerator> successor_generator;

    int get_heuristic_value(const State &ancestor_state);

    bool landmark_is_interesting(
        const State &state, const BitsetView &reached,
        LandmarkNode &lm_node, bool all_lms_reached) const;
    void generate_preferred_operators(
        const State &state, const BitsetView &reached);
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit LandmarkCostPartitioningHeuristic(const plugins::Options &opts);

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override {
        evals.insert(this);
    }

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;
    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
