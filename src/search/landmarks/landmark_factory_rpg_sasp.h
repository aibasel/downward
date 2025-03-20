#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_SASP_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_SASP_H

#include "landmark_factory_relaxation.h"

#include "../utils/hash.h"

#include <deque>
#include <set>
#include <unordered_map>
#include <vector>

namespace landmarks {
class LandmarkFactoryRpgSasp : public LandmarkFactoryRelaxation {
    const bool disjunctive_landmarks;
    const bool use_orders;
    // TODO: Use std::deque instead?
    std::deque<LandmarkNode *> open_landmarks;
    std::vector<std::vector<int>> disjunction_classes;

    std::unordered_map<const LandmarkNode *, utils::HashSet<FactPair>> forward_orders;

    /* The entry `dtg_successors[var][val]` contains all successor values of the
       atom var->val in the domain transition graph (aka atomic projection). */
    std::vector<std::vector<std::unordered_set<int>>> dtg_successors;

    void resize_dtg_data_structures(const TaskProxy &task_proxy);
    void compute_dtg_successors(
        const EffectProxy &effect,
        const std::unordered_map<int, int> &preconditions,
        const std::unordered_map<int, int> &effect_conditions);
    void build_dtg_successors(const TaskProxy &task_proxy);
    void add_dtg_successor(int var_id, int pre, int post);
    void find_forward_orders(const VariablesProxy &variables,
                             const std::vector<std::vector<bool>> &reached,
                             LandmarkNode *node);
    void add_landmark_forward_orderings();

    void compute_shared_preconditions(
        const TaskProxy &task_proxy,
        std::unordered_map<int, int> &shared_pre,
        std::vector<std::vector<bool>> &reached, const Landmark &landmark);
    void compute_disjunctive_preconditions(
        const TaskProxy &task_proxy,
        std::vector<std::set<FactPair>> &disjunctive_pre,
        std::vector<std::vector<bool>> &reached,
        const Landmark &landmark);

    virtual void generate_relaxed_landmarks(
        const std::shared_ptr<AbstractTask> &task,
        Exploration &exploration) override;
    void remove_occurrences_of_landmark_node(const LandmarkNode *node);
    void remove_disjunctive_landmark_and_rewire_orderings(
        LandmarkNode &simple_landmark_node);
    void add_simple_landmark_and_ordering(
        const FactPair &atom, LandmarkNode &node, OrderingType type);
    // TODO: Can we use something different than set in the next two?
    bool deal_with_overlapping_landmarks(
        const std::set<FactPair> &atoms, LandmarkNode &node,
        OrderingType type) const;
    void add_disjunctive_landmark_and_ordering(
        const std::set<FactPair> &atoms, LandmarkNode &node, OrderingType type);
    void approximate_lookahead_orders(
        const TaskProxy &task_proxy,
        const std::vector<std::vector<bool>> &reached, LandmarkNode *node);
    // TODO: Rename this function.
    bool domain_connectivity(const State &initial_state,
                             const FactPair &landmark,
                             const std::unordered_set<int> &exclude);

    void build_disjunction_classes(const TaskProxy &task_proxy);

    void discard_disjunctive_landmarks();
public:
    LandmarkFactoryRpgSasp(bool disjunctive_landmarks, bool use_orders,
                           utils::Verbosity verbosity);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
