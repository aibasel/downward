#ifndef LANDMARKS_LANDMARK_FACTORY_H
#define LANDMARKS_LANDMARK_FACTORY_H

#include "landmark_graph.h"

#include "../utils/logging.h"

#include <vector>

class TaskProxy;

namespace plugins {
class Options;
class Feature;
}

namespace landmarks {
class LandmarkFactory {
    AbstractTask *landmark_graph_task;
    std::vector<std::vector<std::vector<int>>> operators_providing_effect;

    virtual void generate_landmarks(
        const std::shared_ptr<AbstractTask> &task) = 0;
    void log_landmark_graph_info(
        const TaskProxy &task_proxy,
        const utils::Timer &landmark_generation_timer) const;

    void resize_operators_providing_effect(const TaskProxy &task_proxy);
    void add_operator_or_axiom_providing_effects(
        const OperatorProxy &op_or_axiom);
    void compute_operators_providing_effect(const TaskProxy &task_proxy);

    void add_ordering(
        LandmarkNode &from, LandmarkNode &to, OrderingType type) const;

protected:
    mutable utils::LogProxy log;
    std::shared_ptr<LandmarkGraph> landmark_graph;
    bool achievers_calculated = false;

    explicit LandmarkFactory(utils::Verbosity verbosity);

    void add_or_replace_ordering_if_stronger(
        LandmarkNode &from, LandmarkNode &to, OrderingType type) const;

    void discard_all_orderings() const;

    const std::vector<int> &get_operators_including_effect(
        const FactPair &eff) const {
        return operators_providing_effect[eff.var][eff.value];
    }

public:
    virtual ~LandmarkFactory() = default;
    LandmarkFactory(const LandmarkFactory &) = delete;

    std::shared_ptr<LandmarkGraph> compute_landmark_graph(
        const std::shared_ptr<AbstractTask> &task);

    virtual bool supports_conditional_effects() const = 0;

    bool achievers_are_calculated() const {
        return achievers_calculated;
    }
};

extern void add_landmark_factory_options_to_feature(plugins::Feature &feature);
extern std::tuple<utils::Verbosity> get_landmark_factory_arguments_from_options(
    const plugins::Options &opts);
extern void add_use_orders_option_to_feature(plugins::Feature &feature);
extern bool get_use_orders_arguments_from_options(
    const plugins::Options &opts);
}

#endif
