#include "pattern_collection_generator_single_cegar.h"

#include "cegar.h"

#include "../option_parser.h"
#include "../plugin.h"

//#include "../utils/logging.h"
//#include "../utils/rng.h"
#include "../utils/rng_options.h"

using namespace std;

namespace pdbs {
PatternCollectionGeneratorSingleCegar::PatternCollectionGeneratorSingleCegar(
    const options::Options &opts)
    : rng(utils::parse_rng_from_options(opts)),
      max_refinements(opts.get<int>("max_refinements")),
      max_pdb_size(opts.get<int>("max_pdb_size")),
      max_collection_size(opts.get<int>("max_collection_size")),
      wildcard_plans(opts.get<bool>("wildcard_plans")),
      ignore_goal_violations(opts.get<bool>("ignore_goal_violations")),
      global_blacklist_size(opts.get<int>("global_blacklist_size")),
      verbosity(opts.get<utils::Verbosity>("verbosity")),
      max_time(opts.get<double>("max_time")) {
    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << token << "options: " << endl;
        utils::g_log << token << "max refinements: " << max_refinements << endl;
        utils::g_log << token << "max pdb size: " << max_pdb_size << endl;
        utils::g_log << token << "max collection size: " << max_collection_size << endl;
        utils::g_log << token << "wildcard plans: " << wildcard_plans << endl;
        utils::g_log << token << "ignore goal violations: " << ignore_goal_violations << endl;
        utils::g_log << token << "global blacklist size: " << global_blacklist_size << endl;
        utils::g_log << token << "initial collection type: ";
        utils::g_log << token << "Verbosity: ";
        switch (verbosity) {
        case utils::Verbosity::SILENT:
            utils::g_log << "silent";
            break;
        case utils::Verbosity::NORMAL:
            utils::g_log << "normal";
            break;
        case utils::Verbosity::VERBOSE:
            utils::g_log << "verbose";
            break;
        case utils::Verbosity::DEBUG:
            utils::g_log << "debug";
            break;
        }
        utils::g_log << endl;
        utils::g_log << token << "max time: " << max_time << endl;
    }
    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << endl;
    }
}

PatternCollectionGeneratorSingleCegar::~PatternCollectionGeneratorSingleCegar() {
}

PatternCollectionInformation PatternCollectionGeneratorSingleCegar::generate(
    const std::shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    vector<int> goal_variables;
    for (const FactProxy &goal : task_proxy.get_goals()) {
        goal_variables.push_back(goal.get_variable().get_id());
    }

    return cegar(
        task,
        move(goal_variables),
        rng,
        max_refinements,
        max_pdb_size,
        max_collection_size,
        wildcard_plans,
        ignore_goal_violations,
        global_blacklist_size,
        verbosity,
        max_time);
}

static shared_ptr<PatternCollectionGenerator> _parse(
        options::OptionParser& parser) {
    add_cegar_options_to_parser(parser);
    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternCollectionGeneratorSingleCegar>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("single_cegar", _parse);
}
