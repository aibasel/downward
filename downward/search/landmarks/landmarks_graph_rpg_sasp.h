#ifndef LANDMARKS_GRAPH_NEW_H
#define LANDMARKS_GRAPH_NEW_H

#include <ext/hash_set>
#include "../globals.h"
#include "landmarks_graph.h"

class LandmarksGraphNew : public LandmarksGraph {
    list<LandmarkNode *> open_landmarks;

    void find_forward_orders(const vector<vector<int> > &lvl_var,
                             LandmarkNode *lmp);
    void add_lm_forward_orders();

    void get_greedy_preconditions_for_lm(const LandmarkNode *lmp,
                                         const Operator &o, hash_map<int, int> &result) const;
    void compute_shared_preconditions(hash_map<int, int> &shared_pre, vector<
                                          vector<int> > &lvl_var, LandmarkNode *bp);
    void compute_disjunctive_preconditions(
        vector<set<pair<int, int> > > &disjunctive_pre,
        vector<vector<int> > &lvl_var, LandmarkNode *bp);

    int min_cost_for_landmark(LandmarkNode *bp, vector<vector<int> > &lvl_var);
    void generate_landmarks();
    void found_simple_lm_and_order(const pair<int, int> a, LandmarkNode &b,
                                   edge_type t);
    void found_disj_lm_and_order(const set<pair<int, int> > a, LandmarkNode &b,
                                 edge_type t);
    void approximate_lookahead_orders(const vector<vector<int> > &lvl_var,
                                      LandmarkNode *lmp);
    void make_conjunctive_lms();
    static bool domain_connectivity(const pair<int, int> &landmark,
                                    const hash_set<int> &exclude);
public:
    LandmarksGraphNew(Exploration *exploration)
        : LandmarksGraph(exploration) {
    }
    ~LandmarksGraphNew() {
    }
};

#endif
