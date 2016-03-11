#ifndef LANDMARKS_LANDMARK_FACTORY_H
#define LANDMARKS_LANDMARK_FACTORY_H

#include "landmark_graph.h"
#include "exploration.h"

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace landmarks {
class LandmarkFactory {
public:
    LandmarkFactory(const Options &opts);
    virtual ~LandmarkFactory() {}
    // compute_lm_graph *must* be called to avoid memory leeks!
    // returns a landmarkgraph created by a factory class.
    // take care to delete the pointer when you don't need it anymore!
    // (method should principally anyways be called by every inheriting class)
    std::unique_ptr<LandmarkGraph> &&compute_lm_graph();
    Exploration *get_exploration() const {
        assert(exploration);
        return exploration;
    }
    bool use_disjunctive_landmarks() const {return disjunctive_landmarks; }
    bool use_reasonable_orders() const {return reasonable_orders; }
    static void add_options_to_parser(OptionParser &parser);
    bool supports_conditional_effects() {return conditional_effects_supported; }

protected:
    std::unique_ptr<LandmarkGraph> lm_graph;

    bool use_orders() const {return !no_orders; }  // only needed by HMLandmark
    OperatorCost get_lm_cost_type() const {return lm_cost_type; }

    virtual void generate_landmarks() = 0;
    void generate();
    void discard_noncausal_landmarks();
    void discard_disjunctive_landmarks();
    void discard_conjunctive_landmarks();
    void discard_all_orderings();
    inline bool relaxed_task_solvable(bool level_out,
                                      const LandmarkNode *exclude,
                                      bool compute_lvl_op = false) const {
        std::vector<std::vector<int>> lvl_var;
        std::vector<std::unordered_map<std::pair<int, int>, int>> lvl_op;
        return relaxed_task_solvable(lvl_var, lvl_op, level_out, exclude, compute_lvl_op);
    }
    void edge_add(LandmarkNode &from, LandmarkNode &to, edge_type type);
    void compute_predecessor_information(LandmarkNode *bp,
                                         std::vector<std::vector<int>> &lvl_var,
                                         std::vector<std::unordered_map<std::pair<int, int>, int>> &lvl_op);

    // protected not private for LandmarkFactoryRpgSearch
    bool achieves_non_conditional(const GlobalOperator &o, const LandmarkNode *lmp) const;
    bool is_landmark_precondition(const GlobalOperator &o, const LandmarkNode *lmp) const;

private:
    Exploration *exploration;
    int landmarks_count;
    int conj_lms;
    const bool reasonable_orders;
    const bool only_causal_landmarks;
    const bool disjunctive_landmarks;
    const bool conjunctive_landmarks;
    const bool no_orders;
    const OperatorCost lm_cost_type;
    const bool conditional_effects_supported;

    bool interferes(const LandmarkNode *, const LandmarkNode *) const;
    bool effect_always_happens(const std::vector<GlobalEffect> &effects,
                               std::set<std::pair<int, int>> &eff) const;
    void approximate_reasonable_orders(bool obedient_orders);
    void mk_acyclic_graph();
    int loop_acyclic_graph(LandmarkNode &lmn,
                           std::unordered_set<LandmarkNode *> &acyclic_node_set);
    bool remove_first_weakest_cycle_edge(LandmarkNode *cur,
                                         std::list<std::pair<LandmarkNode *, edge_type>> &path,
                                         std::list<std::pair<LandmarkNode *, edge_type>>::iterator it);
    int calculate_lms_cost() const;
    void collect_ancestors(std::unordered_set<LandmarkNode *> &result, LandmarkNode &node,
                           bool use_reasonable);
    bool relaxed_task_solvable(std::vector<std::vector<int>> &lvl_var,
                               std::vector<std::unordered_map<std::pair<int, int>, int>> &lvl_op,
                               bool level_out,
                               const LandmarkNode *exclude,
                               bool compute_lvl_op = false) const;
    bool is_causal_landmark(const LandmarkNode &landmark) const;
    virtual void calc_achievers(); // keep this virtual because HMLandmarks overrides it!
};
}

#endif
