#ifndef LANDMARKS_LANDMARK_COUNT_HEURISTIC_H
#define LANDMARKS_LANDMARK_COUNT_HEURISTIC_H

#include "../heuristic.h"

class BitsetView;

namespace successor_generator {
class SuccessorGenerator;
}

namespace landmarks {
class LandmarkCostAssignment;
class LandmarkGraph;
class LandmarkNode;
class LandmarkStatusManager;

using LandmarkNodeSet = std::unordered_set<const LandmarkNode *>;

class LandmarkCountHeuristic : public Heuristic {
    std::shared_ptr<LandmarkGraph> lgraph;
    const bool use_preferred_operators;
    const bool conditional_effects_supported;
    const bool admissible;
    const bool dead_ends_reliable;

    std::unique_ptr<LandmarkStatusManager> lm_status_manager;
    std::unique_ptr<LandmarkCostAssignment> lm_cost_assignment;
    std::unique_ptr<successor_generator::SuccessorGenerator> successor_generator;

    int get_heuristic_value(const State &ancestor_state);

    bool check_node_orders_disobeyed(
        const LandmarkNode &node, const LandmarkNodeSet &reached) const;

    void add_node_children(LandmarkNode &node,
                           const LandmarkNodeSet &reached) const;

    bool landmark_is_interesting(
        const State &state, const LandmarkNodeSet &reached, LandmarkNode &lm_node) const;
    bool generate_helpful_actions(
        const State &state, const LandmarkNodeSet &reached);

    LandmarkNodeSet convert_to_landmark_set(const BitsetView &landmark_bitset);
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit LandmarkCountHeuristic(const options::Options &opts);

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
