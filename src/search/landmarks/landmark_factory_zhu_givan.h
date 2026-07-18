#ifndef LANDMARKS_LANDMARK_FACTORY_ZHU_GIVAN_H
#define LANDMARKS_LANDMARK_FACTORY_ZHU_GIVAN_H

#include "landmark_factory_relaxation.h"

#include "../utils/hash.h"

#include <unordered_set>
#include <vector>

namespace landmarks {
using LandmarkSet = utils::HashSet<FactPair>;

class LandmarkFactoryZhuGivan : public LandmarkFactoryRelaxation {
    struct PlanGraphNode {
        LandmarkSet labels;
        bool reached() const {
            // NOTE: Reached nodes are always labeled with at least themselves.
            return !labels.empty();
        }
    };

    using PropositionLayer = std::vector<std::vector<PlanGraphNode>>;

    const bool use_orders;

    /* The entry `triggers[i][j]` is a list of operators that could reach/change
       labels on some proposition, after proposition (i,j) has changed. */
    std::vector<std::vector<std::vector<int>>> triggers;

    void compute_triggers(const TaskProxy &task_proxy);

    // Note: must include operators that only have conditional effects.
    std::vector<int> operators_without_preconditions;

    static bool operator_is_applicable(
        const OperatorProxy &op, const PropositionLayer &state);

    static bool conditional_effect_fires(
        const EffectConditionsProxy &effect_conditions,
        const PropositionLayer &layer);

    /* Returns a set of propositions that: (a) have just been reached or (b) had
       their labels changed in next proposition layer. */
    LandmarkSet apply_operator_and_propagate_labels(
        const OperatorProxy &op, const PropositionLayer &current,
        PropositionLayer &next) const;

    // Calculate the union of condition labels from the current layer.
    static LandmarkSet union_of_condition_labels(
        const ConditionsProxy &conditions, const PropositionLayer &current);

    PropositionLayer initialize_relaxed_plan_graph(
        const TaskProxy &task_proxy,
        std::unordered_set<int> &triggered_ops) const;
    void propagate_labels_until_fixed_point_reached(
        const TaskProxy &task_proxy, std::unordered_set<int> &&triggered_ops,
        PropositionLayer &current_layer) const;
    /* Relaxed exploration, returns the last proposition layer
       (the fixpoint) with labels. */
    PropositionLayer build_relaxed_plan_graph_with_labels(
        const TaskProxy &task_proxy) const;

    bool goal_is_reachable(
        const TaskProxy &task_proxy, const PropositionLayer &prop_layer) const;
    LandmarkNode *create_goal_landmark(const FactPair &goal) const;
    void extract_landmarks_and_orderings_from_goal_labels(
        const FactPair &goal, const PropositionLayer &prop_layer,
        LandmarkNode *goal_landmark_node) const;
    /* Construct a landmark graph using the landmarks on the given
       proposition layer. */
    void extract_landmarks(
        const TaskProxy &task_proxy,
        const PropositionLayer &last_prop_layer) const;

    // Link an operators to its propositions in the trigger list.
    void add_operator_to_triggers(const OperatorProxy &op);

    virtual void generate_relaxed_landmarks(
        const std::shared_ptr<AbstractTask> &task,
        Exploration &exploration) override;

public:
    LandmarkFactoryZhuGivan(
        const std::shared_ptr<AbstractTask> &task, bool use_orders,
        utils::Verbosity verbosity);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
