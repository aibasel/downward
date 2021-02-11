#ifndef LANDMARK_FACTORY_HPS_ORDERS_H
#define LANDMARK_FACTORY_HPS_ORDERS_H

#include "landmark_factory.h"

namespace landmarks {
class LandmarkFactoryHPSOrders : public LandmarkFactory {
    std::shared_ptr<LandmarkFactory> lm_factory;

    virtual void generate_landmarks(const std::shared_ptr<AbstractTask> &task) override;

    void approximate_reasonable_orders(
        const TaskProxy &task_proxy, bool obedient_orders);
    bool interferes(
        const TaskProxy &task_proxy, const LandmarkNode *node_a,
        const LandmarkNode *node_b) const;
    void collect_ancestors(
       std::unordered_set<LandmarkNode *> &result,
       LandmarkNode &node, bool use_reasonable);
    bool effect_always_happens(
       const VariablesProxy &variables, const EffectsProxy &effects,
       std::set<FactPair> &eff) const;
public:
    LandmarkFactoryHPSOrders(const options::Options &opts);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
