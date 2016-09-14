#ifndef LANDMARKS_LANDMARK_COUNT_HEURISTIC_H
#define LANDMARKS_LANDMARK_COUNT_HEURISTIC_H

#include "exploration.h"
#include "landmark_graph.h"

#include "../heuristic.h"

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

    std::unique_ptr<LandmarkStatusManager> lm_status_manager;
    std::unique_ptr<LandmarkCostAssignment> lm_cost_assignment;

    int get_heuristic_value(const GlobalState &global_state);

    std::vector<FactPair> collect_lm_leaves(
        bool disjunctive_lms, LandmarkSet &result);

    bool check_node_orders_disobeyed(
        const LandmarkNode &node, const LandmarkSet &reached) const;

    void add_node_children(LandmarkNode &node, const LandmarkSet &reached) const;

    bool landmark_is_interesting(
        const State &s, const LandmarkSet &reached, LandmarkNode &lm) const;
    bool generate_helpful_actions(
        const State &state, const LandmarkSet &reached);
    void set_exploration_goals(const GlobalState &global_state);

    void convert_lms(LandmarkSet &lms_set, const std::vector<bool> &lms_vec);
protected:
    virtual int compute_heuristic(const GlobalState &state) override;
public:
    explicit LandmarkCountHeuristic(const options::Options &opts);
    ~LandmarkCountHeuristic();

    virtual void notify_initial_state(const GlobalState &initial_state) override;
    virtual bool notify_state_transition(const GlobalState &parent_state,
                                         const GlobalOperator &op,
                                         const GlobalState &state) override;
    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
