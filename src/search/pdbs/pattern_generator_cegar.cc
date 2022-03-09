#include "pattern_generator_cegar.h"

#include "cegar.h"
#include "pattern_database.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../utils/logging.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <vector>

using namespace std;

namespace pdbs {
PatternGeneratorCEGAR::PatternGeneratorCEGAR(options::Options &opts)
    : PatternGenerator(opts),
      max_pdb_size(opts.get<int>("max_pdb_size")),
      max_time(opts.get<double>("max_time")),
      use_wildcard_plans(opts.get<bool>("use_wildcard_plans")),
      rng(utils::parse_rng_from_options(opts)) {
}

string PatternGeneratorCEGAR::name() const {
    return "CEGAR pattern generator";
}

PatternInformation PatternGeneratorCEGAR::compute_pattern(
    const shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    vector<FactPair> goals = get_goals_in_random_order(task_proxy, *rng);
    return generate_pattern_with_cegar(
        max_pdb_size,
        max_time,
        use_wildcard_plans,
        log,
        rng,
        task,
        goals[0]);
}

static shared_ptr<PatternGenerator> _parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "CEGAR",
        "This pattern generator uses the CEGAR algorithm restricted to a "
        "random single goal of the task to compute a pattern. See below "
        "for a description of the algorithm and some implementation notes. "
        "The original algorithm (called single CEGAR) is described in the "
        "paper " + get_rovner_et_al_reference());
    add_cegar_implementation_notes_to_parser(parser);
    parser.add_option<int>(
        "max_pdb_size",
        "maximum number of states in the final pattern database (possibly "
        "ignored by a singleton pattern consisting of a single goal variable)",
        "1000000",
        Bounds("1", "infinity"));
    parser.add_option<double>(
        "max_time",
        "maximum time in seconds for the pattern generation",
        "infinity",
        Bounds("0.0", "infinity"));
    add_cegar_wildcard_option_to_parser(parser);
    add_generator_options_to_parser(parser);
    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<PatternGeneratorCEGAR>(opts);
}

static Plugin<PatternGenerator> _plugin("cegar_pattern", _parse);
}
