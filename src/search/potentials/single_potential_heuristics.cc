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

shared_ptr<PotentialFunction> create_potential_function(const Options &opts) {
    PotentialOptimizer optimizer(opts);
    shared_ptr<AbstractTask> task = get_task_from_options(opts);
    TaskProxy task_proxy(*task);
    OptFunc opt_func = static_cast<OptFunc>(opts.get_enum("opt_func"));
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
    if (opts.contains("transform")) {
        options.set<shared_ptr<AbstractTask> >(
            "transform", opts.get<shared_ptr<AbstractTask> >("transform"));
    }
    options.set<int>("cost_type", opts.get<int>("cost_type"));
    options.set<shared_ptr<PotentialFunction> >(
        "function", create_potential_function(opts));
    return new PotentialHeuristic(options);
}

static Plugin<Heuristic> _plugin("single_potentials", _parse);
}
