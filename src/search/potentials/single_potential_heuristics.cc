#include "single_potential_heuristics.h"

#include "potential_function.h"
#include "potential_heuristic.h"
#include "potential_optimizer.h"
#include "util.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;


namespace potentials {
enum class OptimizeFor {
    INITIAL_STATE,
    ALL_STATES,
};

static unique_ptr<PotentialFunction> create_potential_function(
    const Options &opts, OptimizeFor opt_func) {
    PotentialOptimizer optimizer(opts);
    shared_ptr<AbstractTask> task = get_task_from_options(opts);
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

static Heuristic *_parse(OptionParser &parser, OptimizeFor opt_func) {
    prepare_parser_for_admissible_potentials(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return new PotentialHeuristic(
        opts, create_potential_function(opts, opt_func));
}

static Heuristic *_parse_initial_state_potential(OptionParser &parser) {
    parser.document_synopsis(
        "Potential heuristic optimized for initial state", "");
    return _parse(parser, OptimizeFor::INITIAL_STATE);
}

static Heuristic *_parse_all_states_potential(OptionParser &parser) {
    parser.document_synopsis(
        "Potential heuristic optimized for all states", "");
    return _parse(parser, OptimizeFor::ALL_STATES);
}

static Plugin<Heuristic> _plugin_initial_state(
    "initial_state_potential", _parse_initial_state_potential);
static Plugin<Heuristic> _plugin_all_states(
    "all_states_potential", _parse_all_states_potential);
}
