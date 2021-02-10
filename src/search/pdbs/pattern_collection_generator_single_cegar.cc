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
      initial(opts.get<InitialCollectionType>("initial")),
      given_goal(opts.get<int>("given_goal")),
      verbosity(opts.get<utils::Verbosity>("verbosity")),
      max_time(opts.get<double>("max_time")) {
    if (initial == InitialCollectionType::GIVEN_GOAL && given_goal == -1) {
        cerr << "Initial collection type 'given goal', but no goal specified" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }

    if (initial != InitialCollectionType::GIVEN_GOAL && given_goal != -1) {
        cerr << "Goal given, but initial collection type is not set to use it" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }

    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << token << "options: " << endl;
        utils::g_log << token << "max refinements: " << max_refinements << endl;
        utils::g_log << token << "max pdb size: " << max_pdb_size << endl;
        utils::g_log << token << "max collection size: " << max_collection_size << endl;
        utils::g_log << token << "wildcard plans: " << wildcard_plans << endl;
        utils::g_log << token << "ignore goal violations: " << ignore_goal_violations << endl;
        utils::g_log << token << "global blacklist size: " << global_blacklist_size << endl;
        utils::g_log << token << "initial collection type: ";
        switch (initial) {
        case InitialCollectionType::GIVEN_GOAL:
            utils::g_log << "given goal" << endl;
            break;
        case InitialCollectionType::RANDOM_GOAL:
            utils::g_log << "random goal" << endl;
            break;
        case InitialCollectionType::ALL_GOALS:
            utils::g_log << "all goals" << endl;
            break;
        }
        utils::g_log << token << "given goal: " << given_goal << endl;
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
    return cegar(
        task,
        rng,
        max_refinements,
        max_pdb_size,
        max_collection_size,
        wildcard_plans,
        ignore_goal_violations,
        global_blacklist_size,
        initial,
        given_goal,
        verbosity,
        max_time);
}

static shared_ptr<PatternCollectionGenerator> _parse(
        options::OptionParser& parser) {
    add_pattern_collection_generator_cegar_options_to_parser(parser);
    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternCollectionGeneratorSingleCegar>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("single_cegar", _parse);
}
