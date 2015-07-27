#include "single_potential_heuristics.h"

#include "potential_function.h"
#include "potential_heuristic.h"
#include "potential_optimizer.h"
#include "util.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;


namespace potentials {
enum class OptFunc {
    INITIAL_STATE,
    ALL_STATES,
    SAMPLES
};

static shared_ptr<PotentialFunction> create_potential_function(
    const Options &opts, OptFunc opt_func) {
    PotentialOptimizer optimizer(opts);
    shared_ptr<AbstractTask> task = get_task_from_options(opts);
    TaskProxy task_proxy(*task);
    switch (opt_func) {
    case OptFunc::INITIAL_STATE:
        optimizer.optimize_for_state(task_proxy.get_initial_state());
        break;
    case OptFunc::ALL_STATES:
        optimizer.optimize_for_all_states();
        break;
    case OptFunc::SAMPLES:
        optimize_for_samples(optimizer, opts.get<int>("num_samples"));
        break;
    default:
        ABORT("Unkown optimization function");
    }
    return optimizer.get_potential_function();
}

static Heuristic *_parse(OptionParser &parser, OptFunc opt_func) {
    add_common_potentials_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    opts.set<shared_ptr<PotentialFunction> >(
        "function", create_potential_function(opts, opt_func));
    return new PotentialHeuristic(opts);
}

static Heuristic *_parse_initial_state_potential(OptionParser &parser) {
    parser.document_synopsis(
        "Potential heuristic optimized for initial state", "");
    return _parse(parser, OptFunc::INITIAL_STATE);
}

static Heuristic *_parse_all_states_potential(OptionParser &parser) {
    parser.document_synopsis(
        "Potential heuristic optimized for all states", "");
    return _parse(parser, OptFunc::ALL_STATES);
}

static Heuristic *_parse_samples_potential(OptionParser &parser) {
    parser.document_synopsis(
        "Potential heuristic optimized for samples", "");
    parser.add_option<int>(
        "num_samples",
        "Number of states to sample",
        "1000");
    return _parse(parser, OptFunc::SAMPLES);
}

static Plugin<Heuristic> _plugin_initial_state(
    "initial_state_potential", _parse_initial_state_potential);
static Plugin<Heuristic> _plugin_all_states(
    "all_states_potential", _parse_all_states_potential);
static Plugin<Heuristic> _plugin_samples(
    "samples_potential", _parse_samples_potential);
}
