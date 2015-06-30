#include "sample_based_potential_heuristics.h"

#include "potential_heuristics.h"

#include "../plugin.h"

using namespace std;


namespace potentials {
SampleBasedPotentialHeuristics::SampleBasedPotentialHeuristics(const Options &opts)
    : optimizer(opts) {
    for (int i = 0; i < opts.get<int>("num_heuristics"); ++i) {
        optimize_for_samples(optimizer, opts.get<int>("num_samples"));
        heuristics.push_back(optimizer.get_heuristic());
    }
}

vector<shared_ptr<Heuristic> > SampleBasedPotentialHeuristics::get_heuristics() const {
    return heuristics;
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
    options.set<int>("cost_type", 0);
    options.set<vector<shared_ptr<Heuristic> > >(
        "heuristics", factory.get_heuristics());
    return new PotentialHeuristics(options);
}

static Plugin<Heuristic> _plugin("sample_based_potentials", _parse);
}
