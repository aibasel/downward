#include "pattern_collection_generator_single_cegar.h"

#include "cegar.h"

#include "../option_parser.h"
#include "../plugin.h"

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
      blacklist_size(opts.get<int>("blacklist_size")),
      verbosity(opts.get<utils::Verbosity>("verbosity")),
      max_time(opts.get<double>("max_time")) {
}

PatternCollectionGeneratorSingleCegar::~PatternCollectionGeneratorSingleCegar() {
}

PatternCollectionInformation PatternCollectionGeneratorSingleCegar::generate(
    const shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    vector<FactPair> goals;
    for (FactProxy goal : task_proxy.get_goals()) {
        goals.push_back(goal.get_pair());
    }
    rng->shuffle(goals);

    unordered_set<int> blacklisted_variables;
    if (blacklist_size) {
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
             i < min(blacklist_size, static_cast<int>(non_goal_variables.size()));
             ++i) {
            int var_id = non_goal_variables[i];
            blacklisted_variables.insert(var_id);
        }
    }

    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << "Single CEGAR pattern collection generator options: " << endl;
        utils::g_log << "blacklist size: " << blacklist_size << endl;
    }
    return cegar(
        max_refinements,
        max_pdb_size,
        max_collection_size,
        wildcard_plans,
        max_time,
        task,
        move(goals),
        move(blacklisted_variables),
        rng,
        verbosity);
}

static shared_ptr<PatternCollectionGenerator> _parse(
    options::OptionParser &parser) {
    parser.add_option<int>(
        "blacklist_size",
        "Number of randomly selected non-goal variables that are "
        "blacklisted, which means excluded from being added to the pattern "
        "collection. 0 means no blacklisting happens, infinity means "
        "to always exclude all non-goal variables.",
        "0",
        Bounds("0", "infinity")
        );
    utils::add_verbosity_option_to_parser(parser);
    add_cegar_options_to_parser(parser);
    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternCollectionGeneratorSingleCegar>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("single_cegar", _parse);
}
