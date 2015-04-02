#ifndef LANDMARKS_LANDMARK_COUNT_HEURISTIC_H
#define LANDMARKS_LANDMARK_COUNT_HEURISTIC_H

#include "exploration.h"
#include "landmark_cost_assignment.h"
#include "landmark_graph.h"
#include "landmark_status_manager.h"

#include "../global_state.h"
#include "../heuristic.h"

class LandmarkCountHeuristic : public Heuristic {
    friend class LamaFFSynergy;
    LandmarkGraph &lgraph;
    Exploration *exploration;
    bool use_preferred_operators;
    int lookahead;
    bool ff_search_disjunctive_lms;

    LandmarkStatusManager lm_status_manager;
    LandmarkCostAssignment *lm_cost_assignment;

    bool use_cost_sharing;

    int get_heuristic_value(const GlobalState &state);

    void collect_lm_leaves(bool disjunctive_lms, LandmarkSet &result, std::vector<
                               std::pair<int, int> > &leaves);
    bool ff_search_lm_leaves(bool disjunctive_lms, const GlobalState &state,
                             LandmarkSet &result);
    // returns true iff relaxed reachable and marks relaxed operators

    bool check_node_orders_disobeyed(LandmarkNode &node,
                                     const LandmarkSet &reached) const;

    void add_node_children(LandmarkNode &node, const LandmarkSet &reached) const;

    bool landmark_is_interesting(const GlobalState &s, const LandmarkSet &reached,
                                 LandmarkNode &lm) const;
    bool generate_helpful_actions(const GlobalState &state,
                                  const LandmarkSet &reached);
    void set_exploration_goals(const GlobalState &state);

    Exploration *get_exploration() {return exploration; }
    void convert_lms(LandmarkSet &lms_set, const std::vector<bool> &lms_vec);
protected:
    virtual int compute_heuristic(const GlobalState &state);
public:
    LandmarkCountHeuristic(const Options &opts);
    ~LandmarkCountHeuristic() {
    }
    virtual bool reach_state(const GlobalState &parent_state, const GlobalOperator &op,
                             const GlobalState &state);
    virtual bool dead_ends_are_reliable() const;
};

#endif
