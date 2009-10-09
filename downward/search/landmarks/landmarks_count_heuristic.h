/*********************************************************************
 * Authors: Matthias Westphal (westpham@informatik.uni-freiburg.de),
 *          Silvia Richter (silvia.richter@nicta.com.au)
 * (C) Copyright 2008 Matthias Westphal and NICTA
 *
 * This file is part of LAMA.
 *
 * LAMA is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the license, or (at your option) any later version.
 *
 * LAMA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 *********************************************************************/

#ifndef LANDMARKS_COUNT_HEURISTIC_H
#define LANDMARKS_COUNT_HEURISTIC_H

#include "../state.h"
#include "../heuristic.h"
#include "landmarks_graph.h"
#include "../best_first_search.h"
#include "exploration.h"
#include "landmark_status_manager.h"
#include "landmark_cost_assignment.h"

extern LandmarksGraph *g_lgraph; // Make global so graph does not need to be built more than once
                           // even when iterating search (TODO: clean up use of g_lgraph vs.
                           // lgraph in this class).
/*
class LMAuxNodeInfo : public AuxNodeInfo {
 public:
  LandmarkSet reached_lms;
  int reached_lms_cost;
  LMAuxNodeInfo() : AuxNodeInfo(), reached_lms_cost(0) {}
  LMAuxNodeInfo(LandmarkSet previous_lms, int previous_cost) : AuxNodeInfo(), reached_lms(previous_lms),
    reached_lms_cost(previous_cost) {}
  ~LMAuxNodeInfo() {}
};
*/

class LandmarksCountHeuristic : public Heuristic {

    Exploration* exploration;
    LandmarksGraph &lgraph;
    bool preferred_operators;
    int lookahead;
    bool ff_search_disjunctive_lms;

    lm_set goal;
    LandmarkSet initial_state_landmarks;
    LandmarkStausManager lm_status_manager;
    LandmarkCostAssignment *lm_cost_assignment;

    bool use_dynamic_cost_sharing;
    bool use_shared_cost;
    bool use_action_landmark_count;

    int get_heuristic_value(const State& state);

    void collect_lm_leaves(bool disjunctive_lms,
			   LandmarkSet& result,
			   vector<pair<int, int> >& leaves);
    int ff_search_lm_leaves(bool disjunctive_lms, const State& state,
			    LandmarkSet& result);

    bool check_node_orders_disobeyed(LandmarkNode& node,
				     const LandmarkSet& reached) const;

    void add_node_children(LandmarkNode& node,
			   const LandmarkSet& reached) const;

    bool landmark_is_interesting(const State& s, const LandmarkSet& reached, LandmarkNode& lm) const;
    bool generate_helpful_actions(const State& state,
				  const LandmarkSet& reached);
    void set_recompute_heuristic(const State& state);

    //int get_needed_landmarks(const State& state, LandmarkSet& needed) const;

protected:
    virtual int compute_heuristic(const State &state);
    virtual void initialize();
public:
    LandmarksCountHeuristic(bool use_preferred_operators, bool admissible);
    ~LandmarksCountHeuristic() {}
    virtual bool reach_state(const State& parent_state, const Operator &op,
            		const State& state);
    virtual bool dead_ends_are_reliable() {return true;}
    /*virtual bool needs_aux_info() {return true;}
    virtual AuxNodeInfo *get_successor_info(const State& current_state,
					    const Operator *current_operator,
					    const AuxNodeInfo *predecessor_info);
					    */
};

#endif
