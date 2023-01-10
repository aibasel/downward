#ifndef LANDMARKS_LANDMARK_FACTORY_ZHU_GIVAN_H
#define LANDMARKS_LANDMARK_FACTORY_ZHU_GIVAN_H

#include "landmark_factory_relaxation.h"

#include "../utils/hash.h"

#include <unordered_set>
#include <utility>
#include <vector>

namespace landmarks {
using lm_set = utils::HashSet<FactPair>;

class LandmarkFactoryZhuGivan : public LandmarkFactoryRelaxation {
    class plan_graph_node {
public:
        lm_set labels;
        inline bool reached() const {
            // NOTE: nodes are always labeled with itself,
            // if they have been reached
            return !labels.empty();
        }
    };

    using PropositionLayer = std::vector<std::vector<plan_graph_node>>;

    const bool use_orders;

    // triggers[i][j] is a list of operators that could reach/change
    // labels on some proposition, after proposition (i,j) has changed
    std::vector<std::vector<std::vector<int>>> triggers;

    void compute_triggers(const TaskProxy &task_proxy);

    // Note: must include operators that only have conditional effects
    std::vector<int> operators_without_preconditions;

    bool operator_applicable(const OperatorProxy &op, const PropositionLayer &state) const;

    bool operator_cond_effect_fires(const EffectConditionsProxy &effect_conditions,
                                    const PropositionLayer &layer) const;

    // Apply operator and propagate labels to next layer. Returns set of
    // propositions that:
    // (a) have just been reached OR (b) had their labels changed in next
    // proposition layer
    lm_set apply_operator_and_propagate_labels(const OperatorProxy &op,
                                               const PropositionLayer &current, PropositionLayer &next) const;

    // Calculate the union of precondition labels of op, using the
    // labels from current
    lm_set union_of_precondition_labels(const OperatorProxy &op,
                                        const PropositionLayer &current) const;

    // Calculate the union of precondition labels of a conditional effect,
    // using the labels from current
    lm_set union_of_condition_labels(const EffectConditionsProxy &effect_conditions,
                                     const PropositionLayer &current) const;

    // Relaxed exploration, returns the last proposition layer
    // (the fixpoint) with labels
    PropositionLayer build_relaxed_plan_graph_with_labels(const TaskProxy &task_proxy) const;

    // Extract landmarks from last proposition layer and add them to the
    // landmarks graph
    void extract_landmarks(const TaskProxy &task_proxy,
                           const PropositionLayer &last_prop_layer);

    // Link operators to its propositions in trigger list.
    void add_operator_to_triggers(const OperatorProxy &op);

    virtual void generate_relaxed_landmarks(
        const std::shared_ptr<AbstractTask> &task,
        Exploration &exploration) override;

public:
    explicit LandmarkFactoryZhuGivan(const plugins::Options &opts);

    virtual bool computes_reasonable_orders() const override;
    virtual bool supports_conditional_effects() const override;
};
}

#endif
