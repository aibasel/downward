#ifndef LANDMARKS_LANDMARK_FACTORY_H
#define LANDMARKS_LANDMARK_FACTORY_H

#include "landmark_graph.h"

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
/*
  TODO: Change order to private -> protected -> public
   (omitted so far to minimize diff)
*/
class LandmarkFactory {
public:
    virtual ~LandmarkFactory() = default;
    LandmarkFactory(const LandmarkFactory &) = delete;

    std::shared_ptr<LandmarkGraph> compute_lm_graph(const std::shared_ptr<AbstractTask> &task);

    bool use_disjunctive_landmarks() const {return disjunctive_landmarks;}
    bool use_reasonable_orders() const {return reasonable_orders;}
    virtual bool supports_conditional_effects() const = 0;

protected:
    explicit LandmarkFactory(const options::Options &opts);

    std::shared_ptr<LandmarkGraph> lm_graph;
    const bool reasonable_orders;
    const bool only_causal_landmarks;
    const bool disjunctive_landmarks;
    const bool conjunctive_landmarks;
    const bool no_orders;

    /*
      TODO: Make access of member variables of this class consistent (currently,
       no_orders is the only member variable that is accessed via a method,
       while all other are accessed directly.
    */
    bool use_orders() const {return !no_orders;}   // only needed by HMLandmark

    void edge_add(LandmarkNode &from, LandmarkNode &to, EdgeType type);

    void discard_disjunctive_landmarks();
    void discard_conjunctive_landmarks();
    void discard_all_orderings();
    void approximate_reasonable_orders(
        const TaskProxy &task_proxy, bool obedient_orders);
    void mk_acyclic_graph();
    int calculate_lms_cost() const;

    bool is_landmark_precondition(const OperatorProxy &op, const LandmarkNode *lmp) const;

    const std::vector<int> &get_operators_including_eff(const FactPair &eff) const {
        return operators_eff_lookup[eff.var][eff.value];
    }

private:
    AbstractTask *lm_graph_task;

    virtual void generate_landmarks(const std::shared_ptr<AbstractTask> &task) = 0;

    std::vector<std::vector<std::vector<int>>> operators_eff_lookup;

    bool interferes(const TaskProxy &task_proxy,
                    const LandmarkNode *node_a,
                    const LandmarkNode *node_b) const;
    bool effect_always_happens(const VariablesProxy &variables,
                               const EffectsProxy &effects,
                               std::set<FactPair> &eff) const;
    int loop_acyclic_graph(LandmarkNode &lmn,
                           std::unordered_set<LandmarkNode *> &acyclic_node_set);
    bool remove_first_weakest_cycle_edge(LandmarkNode *cur,
                                         std::list<std::pair<LandmarkNode *, EdgeType>> &path,
                                         std::list<std::pair<LandmarkNode *, EdgeType>>::iterator it);
    void collect_ancestors(std::unordered_set<LandmarkNode *> &result, LandmarkNode &node,
                           bool use_reasonable);
    void generate_operators_lookups(const TaskProxy &task_proxy);
};

extern void _add_options_to_parser(options::OptionParser &parser);
}

#endif
