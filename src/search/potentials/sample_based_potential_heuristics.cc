#include "sample_based_potential_heuristics.h"

#include "admissible_potential_heuristics.h"
#include "potential_heuristics.h"

#include "../plugin.h"

using namespace std;


namespace potentials {

SampleBasedPotentialHeuristics::SampleBasedPotentialHeuristics(const Options &opts)
    : optimizer(opts) {
    for (int i = 0; i < opts.get<int>("num_heuristics"); ++i) {
        optimize_for_samples(optimizer, opts.get<int>("num_samples"));
        functions.push_back(optimizer.get_potential_function());
    }
}

vector<shared_ptr<PotentialFunction> > SampleBasedPotentialHeuristics::get_functions() const {
    return functions;
}

static Heuristic *_parse(OptionParser &parser) {
    add_lp_solver_option_to_parser(parser);
    add_common_potentials_options_to_parser(parser);
    Heuristic::add_options_to_parser(parser);
    parser.add_option<int>(
        "num_heuristics",
        "number of potential heuristics",
        "1");
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    SampleBasedPotentialHeuristics factory(opts);

    Options options;
    options.set<int>("cost_type", NORMAL);
    options.set<vector<shared_ptr<PotentialFunction> > >(
        "functions", factory.get_functions());
    return new PotentialHeuristics(options);
}

static Plugin<Heuristic> _plugin("sample_based_potentials", _parse);

}
