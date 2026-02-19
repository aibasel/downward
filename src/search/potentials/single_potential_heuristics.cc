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
    const shared_ptr<AbstractTask> &task, lp::LPSolverType lpsolver,
    double max_potential, OptimizeFor opt_func) {
    PotentialOptimizer optimizer(task, lpsolver, max_potential);
    TaskProxy task_proxy(*task);
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

class TaskIndependentSinglePotentialHeuristic
    : public TaskIndependentEvaluator {
    lp::LPSolverType lpsolver;
    double max_potential;
    OptimizeFor opt_func;
    bool cache_estimates;
    string description;
    utils::Verbosity verbosity;

public:
    TaskIndependentSinglePotentialHeuristic(
        lp::LPSolverType lpsolver, double max_potential, OptimizeFor opt_func,
        bool cache_estimates, const string &description,
        utils::Verbosity verbosity)
        : lpsolver(lpsolver),
          max_potential(max_potential),
          opt_func(opt_func),
          cache_estimates(cache_estimates),
          description(description),
          verbosity(verbosity) {
    }

    virtual std::shared_ptr<Evaluator> create_task_specific_component(
        const std::shared_ptr<AbstractTask> &task, Cache &) const override {
        unique_ptr<PotentialFunction> potential_function =
            create_potential_function(task, lpsolver, max_potential, opt_func);
        return make_shared<PotentialHeuristic>(
            task, move(potential_function), cache_estimates, description,
            verbosity);
    }
};

class InitialStatePotentialHeuristicFeature
    : public plugins::TaskIndependentFeature<TaskIndependentEvaluator> {
public:
    InitialStatePotentialHeuristicFeature()
        : TaskIndependentFeature("initial_state_potential") {
        document_subcategory("heuristics_potentials");
        document_title("Potential heuristic optimized for initial state");
        document_synopsis(get_admissible_potentials_reference());

        add_admissible_potentials_options_to_feature(
            *this, "initial_state_potential");
    }

    virtual shared_ptr<TaskIndependentEvaluator> create_component(
        const plugins::Options &opts) const override {
        return make_shared<TaskIndependentSinglePotentialHeuristic>(
            opts.get<lp::LPSolverType>("lpsolver"),
            opts.get<double>("max_potential"), OptimizeFor::INITIAL_STATE,
            opts.get<bool>("cache_estimates"), opts.get<string>("description"),
            opts.get<utils::Verbosity>("verbosity"));
    }
};

static plugins::FeaturePlugin<InitialStatePotentialHeuristicFeature>
    _plugin_initial_state;

class AllStatesPotentialHeuristicFeature
    : public plugins::TaskIndependentFeature<TaskIndependentEvaluator> {
public:
    AllStatesPotentialHeuristicFeature()
        : TaskIndependentFeature("all_states_potential") {
        document_subcategory("heuristics_potentials");
        document_title("Potential heuristic optimized for all states");
        document_synopsis(get_admissible_potentials_reference());

        add_admissible_potentials_options_to_feature(
            *this, "all_states_potential");
    }

    virtual shared_ptr<TaskIndependentEvaluator> create_component(
        const plugins::Options &opts) const override {
        return make_shared<TaskIndependentSinglePotentialHeuristic>(
            opts.get<lp::LPSolverType>("lpsolver"),
            opts.get<double>("max_potential"), OptimizeFor::ALL_STATES,
            opts.get<bool>("cache_estimates"), opts.get<string>("description"),
            opts.get<utils::Verbosity>("verbosity"));
    }
};

static plugins::FeaturePlugin<AllStatesPotentialHeuristicFeature>
    _plugin_all_states;
}
