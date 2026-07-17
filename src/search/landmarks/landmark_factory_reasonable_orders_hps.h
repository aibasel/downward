#ifndef LANDMARKS_LANDMARK_FACTORY_REASONABLE_ORDERS_HPS_H
#define LANDMARKS_LANDMARK_FACTORY_REASONABLE_ORDERS_HPS_H

#include "landmark_factory.h"

#include <unordered_set>

namespace landmarks {
class LandmarkFactoryReasonableOrdersHPS : public LandmarkFactory {
    std::shared_ptr<LandmarkFactory> landmark_factory;

    virtual void generate_landmarks(
        const std::shared_ptr<AbstractTask> &task) override;

    void approximate_goal_orderings(
        const TaskProxy &task_proxy, LandmarkNode &node) const;
    void insert_reasonable_orderings(
        const TaskProxy &task_proxy,
        const std::unordered_set<LandmarkNode *> &candidates,
        LandmarkNode &node, const Landmark &landmark) const;
    void approximate_reasonable_orderings(const TaskProxy &task_proxy);
    utils::HashSet<FactPair> get_shared_effects_of_achievers(
        const FactPair &atom, const TaskProxy &task_proxy) const;
    bool interferes(
        const VariablesProxy &variables, const TaskProxy &task_proxy,
        const Landmark &landmark_a, const FactPair &atom_a, const FactProxy &a,
        const FactProxy &b) const;
    bool interferes(
        const TaskProxy &task_proxy, const Landmark &landmark_a,
        const Landmark &landmark_b) const;
public:
    LandmarkFactoryReasonableOrdersHPS(
        const std::shared_ptr<AbstractTask> &task,
        const std::shared_ptr<LandmarkFactory> &lm_factory,
        utils::Verbosity verbosity);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
