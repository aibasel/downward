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
    const plugins::Options &opts, OptimizeFor opt_func) {
    PotentialOptimizer optimizer(opts);
    const AbstractTask &task = *opts.get<shared_ptr<AbstractTask>>("transform");
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

class InitialStatePotentialHeuristicFeature : public plugins::TypedFeature<Evaluator, PotentialHeuristic> {
public:
    InitialStatePotentialHeuristicFeature() : TypedFeature("initial_state_potential") {
        document_subcategory("heuristics_potentials");
        document_title("Potential heuristic optimized for initial state");
        document_synopsis(get_admissible_potentials_reference());

        prepare_parser_for_admissible_potentials(*this);
    }

    virtual shared_ptr<PotentialHeuristic> create_component(const plugins::Options &options, const utils::Context &) const override {
        return make_shared<PotentialHeuristic>(options, create_potential_function(options, OptimizeFor::INITIAL_STATE));
    }
};

static plugins::FeaturePlugin<InitialStatePotentialHeuristicFeature> _plugin_initial_state;

class AllStatesPotentialHeuristicFeature : public plugins::TypedFeature<Evaluator, PotentialHeuristic> {
public:
    AllStatesPotentialHeuristicFeature() : TypedFeature("all_states_potential") {
        document_subcategory("heuristics_potentials");
        document_title("Potential heuristic optimized for all states");
        document_synopsis(get_admissible_potentials_reference());

        prepare_parser_for_admissible_potentials(*this);
    }

    virtual shared_ptr<PotentialHeuristic> create_component(const plugins::Options &options, const utils::Context &) const override {
        return make_shared<PotentialHeuristic>(options, create_potential_function(options, OptimizeFor::ALL_STATES));
    }
};

static plugins::FeaturePlugin<AllStatesPotentialHeuristicFeature> _plugin_all_states;
}
