#ifndef LANDMARKS_LANDMARK_FACTORY_H
#define LANDMARKS_LANDMARK_FACTORY_H

#include "landmark_graph.h"

#include "../utils/logging.h"

#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class TaskProxy;

namespace plugins {
class Options;
class Feature;
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

    virtual bool supports_conditional_effects() const = 0;

    bool achievers_are_calculated() const {
        return achievers_calculated;
    }

protected:
    explicit LandmarkFactory(utils::Verbosity verbosity);
    mutable utils::LogProxy log;
    std::shared_ptr<LandmarkGraph> lm_graph;
    bool achievers_calculated = false;

    void edge_add(LandmarkNode &from, LandmarkNode &to, EdgeType type);

    void discard_all_orderings();

    bool is_landmark_precondition(const OperatorProxy &op,
                                  const Landmark &landmark) const;

    const std::vector<int> &get_operators_including_eff(const FactPair &eff) const {
        return operators_eff_lookup[eff.var][eff.value];
    }

private:
    AbstractTask *lm_graph_task;
    std::vector<std::vector<std::vector<int>>> operators_eff_lookup;

    virtual void generate_landmarks(const std::shared_ptr<AbstractTask> &task) = 0;
    void generate_operators_lookups(const TaskProxy &task_proxy);
};

extern void add_landmark_factory_options_to_feature(plugins::Feature &feature);
extern std::tuple<utils::Verbosity> get_landmark_factory_arguments_from_options(
    const plugins::Options &opts);
extern void add_use_orders_option_to_feature(plugins::Feature &feature);
extern bool get_use_orders_arguments_from_options(
    const plugins::Options &opts);
}

#endif
