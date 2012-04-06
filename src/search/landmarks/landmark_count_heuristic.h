#ifndef LANDMARKS_LANDMARK_COUNT_HEURISTIC_H
#define LANDMARKS_LANDMARK_COUNT_HEURISTIC_H

#include "../state.h"
#include "../heuristic.h"
#include "landmark_graph.h"
#include "exploration.h"
#include "landmark_status_manager.h"
#include "landmark_cost_assignment.h"

extern LandmarkGraph *g_lgraph; // Make global so graph does not need to be built more than once
// even when iterating search (TODO: clean up use of g_lgraph vs.
// lgraph in this class).

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

    int get_heuristic_value(const State &state);

    void collect_lm_leaves(bool disjunctive_lms, LandmarkSet &result, vector<
                               pair<int, int> > &leaves);
    bool ff_search_lm_leaves(bool disjunctive_lms, const State &state,
                             LandmarkSet &result);
    // returns true iff relaxed reachable and marks relaxed operators

    bool check_node_orders_disobeyed(LandmarkNode &node,
                                     const LandmarkSet &reached) const;

    void add_node_children(LandmarkNode &node, const LandmarkSet &reached) const;

    bool landmark_is_interesting(const State &s, const LandmarkSet &reached,
                                 LandmarkNode &lm) const;
    bool generate_helpful_actions(const State &state,
                                  const LandmarkSet &reached);
    void set_exploration_goals(const State &state);

    //int get_needed_landmarks(const State& state, LandmarkSet& needed) const;
    Exploration *get_exploration() {return exploration; }
    void convert_lms(LandmarkSet &lms_set, const vector<bool> &lms_vec);
protected:
    virtual int compute_heuristic(const State &state);
public:
    LandmarkCountHeuristic(const Options &opts);
    ~LandmarkCountHeuristic() {
    }
    virtual bool reach_state(const State &parent_state, const Operator &op,
                             const State &state);
    virtual bool dead_ends_are_reliable() const {
        return true;
    }

    virtual void reset();
};

#endif
