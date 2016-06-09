#ifndef LANDMARKS_LANDMARK_FACTORY_H
#define LANDMARKS_LANDMARK_FACTORY_H

#include "../operator_cost.h"

#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct GlobalEffect;

namespace options {
class OptionParser;
class Options;
}

// HACK! remove this once landmark heuristics are switched to the new task interface
#include "../global_state.h"
const GlobalState &hacked_initial_state();


namespace landmarks {
class Exploration;
class LandmarkGraph;
class LandmarkNode;
enum class EdgeType;

class LandmarkFactory {
public:
    explicit LandmarkFactory(const options::Options &opts);
    virtual ~LandmarkFactory() = default;

    std::shared_ptr<LandmarkGraph> compute_lm_graph(Exploration &exploration);

    bool use_disjunctive_landmarks() const {return disjunctive_landmarks; }
    bool use_reasonable_orders() const {return reasonable_orders; }
    virtual bool supports_conditional_effects() const = 0;

protected:
    std::shared_ptr<LandmarkGraph> lm_graph;

    bool use_orders() const {return !no_orders; }  // only needed by HMLandmark
    OperatorCost get_lm_cost_type() const {return lm_cost_type; }

    virtual void generate_landmarks(Exploration &exploration) = 0;
    void generate(Exploration &exploration);
    void discard_noncausal_landmarks(Exploration &exploration);
    void discard_disjunctive_landmarks();
    void discard_conjunctive_landmarks();
    void discard_all_orderings();
    inline bool relaxed_task_solvable(Exploration &exploration,
                                      bool level_out,
                                      const LandmarkNode *exclude,
                                      bool compute_lvl_op = false) const {
        std::vector<std::vector<int>> lvl_var;
        std::vector<std::unordered_map<std::pair<int, int>, int>> lvl_op;
        return relaxed_task_solvable(exploration, lvl_var, lvl_op, level_out, exclude, compute_lvl_op);
    }
    void edge_add(LandmarkNode &from, LandmarkNode &to, EdgeType type);
    void compute_predecessor_information(Exploration &exploration,
                                         LandmarkNode *bp,
                                         std::vector<std::vector<int>> &lvl_var,
                                         std::vector<std::unordered_map<std::pair<int, int>, int>> &lvl_op);

    // protected not private for LandmarkFactoryRpgSearch
    bool achieves_non_conditional(const GlobalOperator &o, const LandmarkNode *lmp) const;
    bool is_landmark_precondition(const GlobalOperator &o, const LandmarkNode *lmp) const;

private:
    const bool reasonable_orders;
    const bool only_causal_landmarks;
    const bool disjunctive_landmarks;
    const bool conjunctive_landmarks;
    const bool no_orders;
    const OperatorCost lm_cost_type;

    bool interferes(const LandmarkNode *, const LandmarkNode *) const;
    bool effect_always_happens(const std::vector<GlobalEffect> &effects,
                               std::set<std::pair<int, int>> &eff) const;
    void approximate_reasonable_orders(bool obedient_orders);
    void mk_acyclic_graph();
    int loop_acyclic_graph(LandmarkNode &lmn,
                           std::unordered_set<LandmarkNode *> &acyclic_node_set);
    bool remove_first_weakest_cycle_edge(LandmarkNode *cur,
                                         std::list<std::pair<LandmarkNode *, EdgeType>> &path,
                                         std::list<std::pair<LandmarkNode *, EdgeType>>::iterator it);
    int calculate_lms_cost() const;
    void collect_ancestors(std::unordered_set<LandmarkNode *> &result, LandmarkNode &node,
                           bool use_reasonable);
    bool relaxed_task_solvable(Exploration &exploration,
                               std::vector<std::vector<int>> &lvl_var,
                               std::vector<std::unordered_map<std::pair<int, int>, int>> &lvl_op,
                               bool level_out,
                               const LandmarkNode *exclude,
                               bool compute_lvl_op = false) const;
    bool is_causal_landmark(Exploration &exploration, const LandmarkNode &landmark) const;
    virtual void calc_achievers(Exploration &exploration); // keep this virtual because HMLandmarks overrides it!
};

extern void _add_options_to_parser(options::OptionParser &parser);
}

#endif
