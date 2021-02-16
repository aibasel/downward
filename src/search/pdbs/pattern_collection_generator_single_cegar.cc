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
    vector<FactPair> goals;
    for (FactProxy goal : task_proxy.get_goals()) {
        goals.push_back(goal.get_pair());
    }
    rng->shuffle(goals);

    unordered_set<int> blacklisted_variables;
    if (global_blacklist_size) {
        int num_vars = task_proxy.get_variables().size();
        vector<int> non_goal_variables;
        non_goal_variables.reserve(num_vars - goals.size());
        for (int var_id = 0; var_id < num_vars; ++var_id) {
            bool is_goal_var = false;
            for (const FactPair &goal : goals) {
                if (var_id == goal.var) {
                    is_goal_var = true;
                    break;
                }
            }
            if (!is_goal_var) {
                non_goal_variables.push_back(var_id);
            }
        }
        rng->shuffle(non_goal_variables);

        // Select a random subset of non goals.
        for (int i = 0;
            i < min(global_blacklist_size, static_cast<int>(non_goal_variables.size()));
            ++i) {
            int var_id = non_goal_variables[i];
            if (verbosity >= utils::Verbosity::VERBOSE) {
                utils::g_log << token << "blacklisting var" << var_id << endl;
            }
            blacklisted_variables.insert(var_id);
        }
    }

    return cegar(
        task,
        move(goals),
        rng,
        max_refinements,
        max_pdb_size,
        max_collection_size,
        wildcard_plans,
        ignore_goal_violations,
        verbosity,
        max_time,
        move(blacklisted_variables));
}

static shared_ptr<PatternCollectionGenerator> _parse(
        options::OptionParser& parser) {
    parser.add_option<int>(
        "global_blacklist_size",
        "Number of randomly selected non-goal variables that are globally "
        "blacklisted, which means excluded from being added to the pattern "
        "collection. 0 means no global blacklisting happens, infinity means "
        "to always exclude all non-goal variables.",
        "0",
        Bounds("0", "infinity")
    );
    add_cegar_options_to_parser(parser);
    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternCollectionGeneratorSingleCegar>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("single_cegar", _parse);
}
