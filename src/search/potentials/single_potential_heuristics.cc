#include "potential_function.h"
#include "potential_heuristic.h"
#include "potential_optimizer.h"
#include "util.h"

#include "../plugins/plugin.h"
#include "../utils/system.h"

using namespace std;

namespace potentials {
enum class OptimizeFor {
    INITIAL_STATE,
    ALL_STATES,
};

static unique_ptr<PotentialFunction> create_potential_function(
    const shared_ptr<AbstractTask> &transform,
    lp::LPSolverType lpsolver, double max_potential,
    OptimizeFor opt_func) {
    PotentialOptimizer optimizer(transform, lpsolver, max_potential);
    const AbstractTask &task = *transform;
    TaskProxy task_proxy(task);
    switch (opt_func) {
    case OptimizeFor::INITIAL_STATE:
        optimizer.optimize_for_state(task_proxy.get_initial_state());
        break;
    case OptimizeFor::ALL_STATES:
        optimizer.optimize_for_all_states();
        break;
    default:
        ABORT("Unkown optimization function");
    }
    return optimizer.get_potential_function();
}

class InitialStatePotentialHeuristicFeature
    : public plugins::TypedFeature<Evaluator, PotentialHeuristic> {
public:
    InitialStatePotentialHeuristicFeature() : TypedFeature("initial_state_potential") {
        document_subcategory("heuristics_potentials");
        document_title("Potential heuristic optimized for initial state");
        document_synopsis(get_admissible_potentials_reference());

        add_admissible_potentials_options_to_feature(
            *this, "initial_state_potential");
    }

    virtual shared_ptr<PotentialHeuristic> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return make_shared<PotentialHeuristic>(
            create_potential_function(
                opts.get<shared_ptr<AbstractTask>>("transform"),
                opts.get<lp::LPSolverType>("lpsolver"),
                opts.get<double>("max_potential"),
                OptimizeFor::INITIAL_STATE),
            opts.get<shared_ptr<AbstractTask>>("transform"),
            opts.get<bool>("cache_estimates"),
            opts.get<string>("description"),
            opts.get<utils::Verbosity>("verbosity")
            );
    }
};

static plugins::FeaturePlugin<InitialStatePotentialHeuristicFeature> _plugin_initial_state;

class AllStatesPotentialHeuristicFeature
    : public plugins::TypedFeature<Evaluator, PotentialHeuristic> {
public:
    AllStatesPotentialHeuristicFeature() : TypedFeature("all_states_potential") {
        document_subcategory("heuristics_potentials");
        document_title("Potential heuristic optimized for all states");
        document_synopsis(get_admissible_potentials_reference());

        add_admissible_potentials_options_to_feature(
            *this, "all_states_potential");
    }

    virtual shared_ptr<PotentialHeuristic> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return make_shared<PotentialHeuristic>(
            create_potential_function(
                opts.get<shared_ptr<AbstractTask>>("transform"),
                opts.get<lp::LPSolverType>("lpsolver"),
                opts.get<double>("max_potential"),
                OptimizeFor::ALL_STATES),
            opts.get<shared_ptr<AbstractTask>>("transform"),
            opts.get<bool>("cache_estimates"),
            opts.get<string>("description"),
            opts.get<utils::Verbosity>("verbosity")
            );
    }
};

static plugins::FeaturePlugin<AllStatesPotentialHeuristicFeature> _plugin_all_states;
}
