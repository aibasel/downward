#include "pattern_generator_rcg.h"

#include "pattern_database.h"
#include "rcg.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../utils/logging.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"
#include "../utils/timer.h"

#include <vector>

using namespace std;

namespace pdbs {

PatternGeneratorRCG::PatternGeneratorRCG(
    options::Options& opts)
    : max_pdb_size(opts.get<int>("max_pdb_size")),
      max_time(opts.get<double>("max_time")),
      bidirectional(opts.get<bool>("bidirectional")),
      verbosity(opts.get<utils::Verbosity>("verbosity")),
      rng(utils::parse_rng_from_options(opts)) {
}

PatternInformation PatternGeneratorRCG::generate(
    const std::shared_ptr<AbstractTask> &task) {
    utils::Timer timer;
    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << "Single RCG: generating patterns" << endl;
    }
    vector<vector<int>> cg_neighbors = compute_cg_neighbors(
        task, bidirectional);
    TaskProxy task_proxy(*task);
    vector<FactPair> goals = get_goals_in_random_order(task_proxy, *rng);

    Pattern pattern = generate_pattern_rcg(
        max_pdb_size,
        max_time,
        goals[0].var,
        verbosity,
        rng,
        task_proxy,
        cg_neighbors
    );

    PatternInformation result(task_proxy, pattern);
    if (verbosity >= utils::Verbosity::NORMAL) {
        dump_pattern_generation_statistics(
            "Single RCG",
            timer.stop(),
            result);
    }
    return result;
}

static shared_ptr<PatternGenerator> _parse(options::OptionParser &parser) {
    add_rcg_options_to_parser(parser);
    utils::add_verbosity_option_to_parser(parser);
    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<PatternGeneratorRCG>(opts);
}

static Plugin<PatternGenerator> _plugin("rcg_pattern", _parse);
}
