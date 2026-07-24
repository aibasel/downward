#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_SASP_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_SASP_H

#include "landmark_factory_relaxation.h"

#include "../utils/hash.h"

#include <deque>
#include <unordered_map>
#include <vector>

namespace landmarks {

using DomainTransitionGraph = std::vector<std::unordered_set<int>>;

class DomainTransitionGraphCollection {
    /*
      NOTE: Similar code exists in M&S atomic abstractions (fts_factory.cc),
      in Stubborn sets (stubborn_sets_ec.cc). There is also a class
      /heuristics/domain_transition_graph that is used for cg_heuristics,
      cea_heuristics, and possibly others. We need a more general mechanism for
      creating data structures of this kind.
      NOTE: the class name could be changed...
    */
private:
    /* The entry `graphs[var][val]` contains all successor values of the
    atom var->val in the domain transition graph (aka atomic projection). */
    std::vector<DomainTransitionGraph> graphs;
    void initialize_data(const TaskProxy &task_proxy);
    void build_domain_transition_graphs(const TaskProxy &task_proxy);
    void compute_successors(
        const EffectProxy &effect,
        const std::unordered_map<int, int> &preconditions,
        const std::unordered_map<int, int> &effect_conditions);
    void add_successor(int var_id, int pre, int post);

public:
    DomainTransitionGraphCollection(const TaskProxy &task_proxy);
    DomainTransitionGraph get_domain_transition_graph(int var_id) const;
};

class LandmarkFactoryRpgSasp : public LandmarkFactoryRelaxation {
    const bool disjunctive_landmarks;
    const bool use_orders;
    std::deque<LandmarkNode *> open_landmarks;
    std::vector<std::vector<int>> disjunction_classes;

    std::unordered_map<const LandmarkNode *, utils::HashSet<FactPair>>
        forward_orderings;

    bool atom_and_landmark_achievable_together(
        const FactPair &atom, const Landmark &landmark) const;
    utils::HashSet<FactPair> compute_atoms_unreachable_without_landmark(
        const VariablesProxy &variables, const Landmark &landmark,
        const std::vector<std::vector<bool>> &reached) const;

    void add_landmark_forward_orderings();

    utils::HashSet<FactPair> compute_shared_preconditions(
        const TaskProxy &task_proxy, const Landmark &landmark,
        const std::vector<std::vector<bool>> &reached) const;
    std::vector<int> get_operators_achieving_landmark(
        const Landmark &landmark) const;
    void extend_disjunction_class_lookups(
        const utils::HashSet<FactPair> &landmark_preconditions, int op_id,
        std::unordered_map<int, std::vector<FactPair>> &preconditions,
        std::unordered_map<int, std::unordered_set<int>> &used_operators) const;
    std::vector<utils::HashSet<FactPair>> compute_disjunctive_preconditions(
        const TaskProxy &task_proxy, const Landmark &landmark,
        const std::vector<std::vector<bool>> &reached) const;

    void generate_goal_landmarks(const TaskProxy &task_proxy);
    void generate_shared_precondition_landmarks(
        const TaskProxy &task_proxy, const Landmark &landmark,
        LandmarkNode *node, const std::vector<std::vector<bool>> &reached);
    void generate_disjunctive_precondition_landmarks(
        const TaskProxy &task_proxy, const State &initial_state,
        const Landmark &landmark, LandmarkNode *node,
        const std::vector<std::vector<bool>> &reached);
    void generate_backchaining_landmarks(
        const TaskProxy &task_proxy,
        const DomainTransitionGraphCollection &dtgs, Exploration &exploration);
    virtual void generate_relaxed_landmarks(
        const std::shared_ptr<AbstractTask> &task,
        Exploration &exploration) override;
    void remove_occurrences_of_landmark_node(const LandmarkNode *node);
    void remove_disjunctive_landmark_and_rewire_orderings(
        LandmarkNode &atomic_landmark_node);
    void add_atomic_landmark_and_ordering(
        const FactPair &atom, LandmarkNode &node, OrderingType type);
    bool deal_with_overlapping_landmarks(
        const utils::HashSet<FactPair> &atoms, LandmarkNode &node,
        OrderingType type) const;
    void add_disjunctive_landmark_and_ordering(
        const utils::HashSet<FactPair> &atoms, LandmarkNode &node,
        OrderingType type);
    void approximate_lookahead_orderings(
        const TaskProxy &task_proxy,
        const DomainTransitionGraphCollection &dtgs,
        const std::vector<std::vector<bool>> &reached, LandmarkNode *node);

    void build_disjunction_classes(const TaskProxy &task_proxy);

    void discard_disjunctive_landmarks() const;
public:
    LandmarkFactoryRpgSasp(
        const std::shared_ptr<AbstractTask> &task, bool disjunctive_landmarks,
        bool use_orders, utils::Verbosity verbosity);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
