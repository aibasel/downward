#include "sample_based_potential_heuristics.h"

#include "potential_max_heuristic.h"
#include "potential_optimizer.h"
#include "util.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <memory>
#include <vector>

using namespace std;


namespace potentials {
/*
  Compute multiple potential functions that are optimized for different
  sets of samples.
*/
static vector<shared_ptr<PotentialFunction> > create_sample_based_potential_functions(
    const Options &opts) {
    vector<shared_ptr<PotentialFunction> > functions;
    PotentialOptimizer optimizer(opts);
    for (int i = 0; i < opts.get<int>("num_heuristics"); ++i) {
        optimize_for_samples(optimizer, opts.get<int>("num_samples"));
        functions.push_back(optimizer.get_potential_function());
    }
    return functions;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Sample-based potential heuristics",
        "Maximum over multiple potential heuristics optimized for samples");
    parser.add_option<int>(
        "num_heuristics",
        "number of potential heuristics",
        "1");
    parser.add_option<int>(
        "num_samples",
        "Number of states to sample",
        "1000");
    add_common_potentials_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    opts.set<vector<shared_ptr<PotentialFunction> > >(
        "functions", create_sample_based_potential_functions(opts));
    return new PotentialMaxHeuristic(opts);
}

static Plugin<Heuristic> _plugin("sample_based_potentials", _parse);
}
