#ifndef LANDMARKS_LANDMARK_FACTORY_H
#define LANDMARKS_LANDMARK_FACTORY_H

#include "landmark_graph.h"
#include "exploration.h"

#include <ext/hash_map>
#include <ext/hash_set>
#include <map>
#include <set>
#include <vector>

class LandmarkFactory {
public:
    LandmarkFactory(const Options &opts);
    virtual ~LandmarkFactory() {}
    // compute_lm_graph *must* be called to avoid memory leeks!
    // returns a landmarkgraph created by a factory class.
    // take care to delete the pointer when you don't need it anymore!
    // (method should principally anyways be called by every inheriting class)
    LandmarkGraph *compute_lm_graph();
protected:
    LandmarkGraph *lm_graph;
    virtual void generate_landmarks() = 0;
    void generate();
    void discard_noncausal_landmarks();
    void discard_disjunctive_landmarks();
    void discard_conjunctive_landmarks();
    void discard_all_orderings();
    inline bool relaxed_task_solvable(bool level_out,
                                      const LandmarkNode *exclude,
                                      bool compute_lvl_op = false) const {
        std::vector<std::vector<int> > lvl_var;
        std::vector<__gnu_cxx::hash_map<std::pair<int, int>, int, hash_int_pair> > lvl_op;
        return relaxed_task_solvable(lvl_var, lvl_op, level_out, exclude, compute_lvl_op);
    }
    void edge_add(LandmarkNode &from, LandmarkNode &to, edge_type type);
    void compute_predecessor_information(LandmarkNode *bp,
                                         std::vector<std::vector<int> > &lvl_var,
                                         std::vector<__gnu_cxx::hash_map<std::pair<int, int>, int, hash_int_pair> > &lvl_op);

    // protected not private for LandmarkFactoryRpgSearch
    bool achieves_non_conditional(const Operator &o, const LandmarkNode *lmp) const;
    bool is_landmark_precondition(const Operator &o, const LandmarkNode *lmp) const;

private:
    bool interferes(const LandmarkNode *, const LandmarkNode *) const;
    bool effect_always_happens(const std::vector<PrePost> &prepost,
                               std::set<std::pair<int, int> > &eff) const;
    void approximate_reasonable_orders(bool obedient_orders);
    void mk_acyclic_graph();
    int loop_acyclic_graph(LandmarkNode &lmn,
                           __gnu_cxx::hash_set<LandmarkNode *, hash_pointer> &acyclic_node_set);
    bool remove_first_weakest_cycle_edge(LandmarkNode *cur,
                                         std::list<std::pair<LandmarkNode *, edge_type> > &path,
                                         std::list<std::pair<LandmarkNode *, edge_type> >::iterator it);
    int calculate_lms_cost() const;
    void collect_ancestors(__gnu_cxx::hash_set<LandmarkNode *, hash_pointer> &result, LandmarkNode &node,
                           bool use_reasonable);
    bool relaxed_task_solvable(std::vector<std::vector<int> > &lvl_var,
                               std::vector<__gnu_cxx::hash_map<std::pair<int, int>, int, hash_int_pair> > &lvl_op,
                               bool level_out,
                               const LandmarkNode *exclude,
                               bool compute_lvl_op = false) const;
    bool is_causal_landmark(const LandmarkNode &landmark) const;
    virtual void calc_achievers(); // keep this virtual because HMLandmarks overrides it!
};

#endif
