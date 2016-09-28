#ifndef LANDMARKS_LANDMARK_FACTORY_H
#define LANDMARKS_LANDMARK_FACTORY_H

#include "landmark_graph.h"

#include "../operator_cost.h"

#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class TaskProxy;

namespace options {
class OptionParser;
class Options;
}

namespace landmarks {
class Exploration;
class LandmarkGraph;
class LandmarkNode;
enum class EdgeType;

class LandmarkFactory {
public:
    explicit LandmarkFactory(const options::Options &opts);
    virtual ~LandmarkFactory() = default;

    LandmarkFactory(const LandmarkFactory &) = delete;

    std::shared_ptr<LandmarkGraph> compute_lm_graph(const std::shared_ptr<AbstractTask> &task, Exploration &exploration);

    bool use_disjunctive_landmarks() const {return disjunctive_landmarks; }
    bool use_reasonable_orders() const {return reasonable_orders; }
    virtual bool supports_conditional_effects() const = 0;

protected:
    std::shared_ptr<LandmarkGraph> lm_graph;

    bool use_orders() const {return !no_orders; }  // only needed by HMLandmark

    virtual void generate_landmarks(const std::shared_ptr<AbstractTask> &task, Exploration &exploration) = 0;
    void generate(const TaskProxy &task_proxy, Exploration &exploration);
    void discard_noncausal_landmarks(const TaskProxy &task_proxy, Exploration &exploration);
    void discard_disjunctive_landmarks();
    void discard_conjunctive_landmarks();
    void discard_all_orderings();
    inline bool relaxed_task_solvable(const TaskProxy &task_proxy, Exploration &exploration,
                                      bool level_out,
                                      const LandmarkNode *exclude,
                                      bool compute_lvl_op = false) const {
        std::vector<std::vector<int>> lvl_var;
        std::vector<std::unordered_map<FactPair, int>> lvl_op;
        return relaxed_task_solvable(task_proxy, exploration, lvl_var, lvl_op, level_out, exclude, compute_lvl_op);
    }
    void edge_add(LandmarkNode &from, LandmarkNode &to, EdgeType type);
    void compute_predecessor_information(const TaskProxy &task_proxy,
                                         Exploration &exploration,
                                         LandmarkNode *bp,
                                         std::vector<std::vector<int>> &lvl_var,
                                         std::vector<std::unordered_map<FactPair, int>> &lvl_op);

    // protected not private for LandmarkFactoryRpgSearch
    bool achieves_non_conditional(const OperatorProxy &o, const LandmarkNode *lmp) const;
    bool is_landmark_precondition(const OperatorProxy &o, const LandmarkNode *lmp) const;

private:
    const bool reasonable_orders;
    const bool only_causal_landmarks;
    const bool disjunctive_landmarks;
    const bool conjunctive_landmarks;
    const bool no_orders;
    const OperatorCost lm_cost_type;

    bool interferes(const TaskProxy &task_proxy,
                    const LandmarkNode *lm_node1,
                    const LandmarkNode *node_b) const;
    bool effect_always_happens(const VariablesProxy &variables,
                               const EffectsProxy &effects,
                               std::set<FactPair> &eff) const;
    void approximate_reasonable_orders(
        const TaskProxy &task_proxy, bool obedient_orders);
    void mk_acyclic_graph();
    int loop_acyclic_graph(LandmarkNode &lmn,
                           std::unordered_set<LandmarkNode *> &acyclic_node_set);
    bool remove_first_weakest_cycle_edge(LandmarkNode *cur,
                                         std::list<std::pair<LandmarkNode *, EdgeType>> &path,
                                         std::list<std::pair<LandmarkNode *, EdgeType>>::iterator it);
    int calculate_lms_cost() const;
    void collect_ancestors(std::unordered_set<LandmarkNode *> &result, LandmarkNode &node,
                           bool use_reasonable);
    bool relaxed_task_solvable(const TaskProxy &task_proxy, Exploration &exploration,
                               std::vector<std::vector<int>> &lvl_var,
                               std::vector<std::unordered_map<FactPair, int>> &lvl_op,
                               bool level_out,
                               const LandmarkNode *exclude,
                               bool compute_lvl_op = false) const;
    void add_operator_and_propositions_to_list(const OperatorProxy &op,
                                               std::vector<std::unordered_map<FactPair, int>> &lvl_op) const;
    bool is_causal_landmark(const TaskProxy &task_proxy, Exploration &exploration, const LandmarkNode &landmark) const;
    virtual void calc_achievers(const TaskProxy &task_proxy, Exploration &exploration); // keep this virtual because HMLandmarks overrides it!
};

extern void _add_options_to_parser(options::OptionParser &parser);
}

#endif
