#ifndef LANDMARKS_LANDMARK_COUNT_HEURISTIC_H
#define LANDMARKS_LANDMARK_COUNT_HEURISTIC_H

#include "exploration.h"
#include "landmark_graph.h"

#include "../heuristic.h"

namespace successor_generator {
class SuccessorGenerator;
}

namespace landmarks {
class LandmarkCostAssignment;
class LandmarkStatusManager;

class LandmarkCountHeuristic : public Heuristic {
    friend class LamaFFSynergy;
    std::shared_ptr<LandmarkGraph> lgraph;
    Exploration exploration;
    const bool use_preferred_operators;
    const bool ff_search_disjunctive_lms;
    const bool conditional_effects_supported;
    const bool admissible;
    const bool dead_ends_reliable;

    std::unique_ptr<LandmarkStatusManager> lm_status_manager;
    std::unique_ptr<LandmarkCostAssignment> lm_cost_assignment;
    std::unique_ptr<successor_generator::SuccessorGenerator> successor_generator;

    int get_heuristic_value(const GlobalState &global_state);

    std::vector<FactPair> collect_lm_leaves(
        bool disjunctive_lms, const LandmarkSet &result);

    bool check_node_orders_disobeyed(
        const LandmarkNode &node, const LandmarkSet &reached) const;

    void add_node_children(LandmarkNode &node, const LandmarkSet &reached) const;

    bool landmark_is_interesting(
        const State &state, const LandmarkSet &reached, LandmarkNode &lm) const;
    bool generate_helpful_actions(
        const State &state, const LandmarkSet &reached);
    void set_exploration_goals(const GlobalState &global_state);

    LandmarkSet convert_to_landmark_set(
        const std::vector<bool> &landmark_vector);
protected:
    virtual int compute_heuristic(const GlobalState &state) override;
public:
    explicit LandmarkCountHeuristic(const options::Options &opts);
    ~LandmarkCountHeuristic();

    virtual void notify_initial_state(const GlobalState &initial_state) override;
    virtual bool notify_state_transition(const GlobalState &parent_state,
                                         OperatorID op_id,
                                         const GlobalState &state) override;
    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
