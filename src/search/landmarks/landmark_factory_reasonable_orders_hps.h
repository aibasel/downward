#ifndef LANDMARKS_LANDMARK_FACTORY_REASONABLE_ORDERS_HPS_H
#define LANDMARKS_LANDMARK_FACTORY_REASONABLE_ORDERS_HPS_H

#include "landmark_factory.h"

namespace landmarks {
class LandmarkFactoryReasonableOrdersHPS : public LandmarkFactory {
    std::shared_ptr<LandmarkFactory> lm_factory;

    virtual void generate_landmarks(const std::shared_ptr<AbstractTask> &task) override;

    void approximate_reasonable_orders(
        const TaskProxy &task_proxy, bool obedient_orders);
    bool interferes(
        const TaskProxy &task_proxy, const Landmark &landmark_a,
        const Landmark &landmark_b) const;
    void collect_ancestors(
        std::unordered_set<LandmarkNode *> &result,
        LandmarkNode &node, bool use_reasonable);
    bool effect_always_happens(
        const VariablesProxy &variables, const EffectsProxy &effects,
        std::set<FactPair> &eff) const;
public:
    LandmarkFactoryReasonableOrdersHPS(const plugins::Options &opts);

    virtual bool computes_reasonable_orders() const override;
    virtual bool supports_conditional_effects() const override;
};
}

#endif
