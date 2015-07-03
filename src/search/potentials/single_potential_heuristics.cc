#include "single_potential_heuristics.h"

#include "potential_function.h"
#include "potential_heuristic.h"
#include "potential_optimizer.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../sampling.h"
#include "../state_registry.h"

using namespace std;


namespace potentials {

enum class OptFunc {
    INITIAL_STATE,
    ALL_STATES,
    SAMPLES
};

void filter_dead_ends(PotentialOptimizer &optimizer, vector<GlobalState> &samples) {
    assert(!optimizer.potentials_are_bounded());
    vector<GlobalState> non_dead_end_samples;
    for (const GlobalState &sample : samples) {
        if (optimizer.optimize_for_state(sample))
            non_dead_end_samples.push_back(sample);
    }
    swap(samples, non_dead_end_samples);
}

void optimize_for_samples(PotentialOptimizer &optimizer, int num_samples) {
    StateRegistry sample_registry;
    vector<GlobalState> samples;
    optimizer.optimize_for_state(g_initial_state());
    if (!optimizer.has_optimal_solution()) {
        // Initial state is a dead end.
        return;
    }
    shared_ptr<Heuristic> sampling_heuristic =
        create_potential_heuristic(optimizer.get_potential_function());
    samples = sample_states_with_random_walks(
        sample_registry, num_samples, *sampling_heuristic);
    if (!optimizer.potentials_are_bounded()) {
        filter_dead_ends(optimizer, samples);
    }
    optimizer.optimize_for_samples(samples);
}

shared_ptr<PotentialFunction> create_potential_function(const Options &opts) {
    PotentialOptimizer optimizer(opts);
    OptFunc opt_func(OptFunc(opts.get_enum("opt_func")));
    if (opt_func == OptFunc::INITIAL_STATE) {
        optimizer.optimize_for_state(g_initial_state());
    } else if (opt_func == OptFunc::ALL_STATES) {
        optimizer.optimize_for_all_states();
    } else if (opt_func == OptFunc::SAMPLES) {
        optimize_for_samples(optimizer, opts.get<int>("num_samples"));
    } else {
        ABORT("Unkown optimization function");
    }
    return optimizer.get_potential_function();
}

static Heuristic *_parse(OptionParser &parser) {
    vector<string> opt_funcs;
    vector<string> opt_funcs_doc;
    opt_funcs.push_back("INITIAL_STATE");
    opt_funcs_doc.push_back(
        "optimize heuristic for initial state");
    opt_funcs.push_back("ALL_STATES");
    opt_funcs_doc.push_back(
        "optimize heuristic for all states");
    opt_funcs.push_back("SAMPLES");
    opt_funcs_doc.push_back(
        "optimize heuristic for a set of sample states");
    parser.add_enum_option(
        "opt_func",
        opt_funcs,
        "Optimization function",
        "SAMPLES",
        opt_funcs_doc);
    add_common_potentials_options_to_parser(parser);
    add_lp_solver_option_to_parser(parser);
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    Options options;
    options.set<int>("cost_type", NORMAL);
    options.set<shared_ptr<PotentialFunction> >(
        "function", create_potential_function(opts));
    return new PotentialHeuristic(options);
}

static Plugin<Heuristic> _plugin("single_potentials", _parse);

}
