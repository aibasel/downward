#include "pattern_collection_generator_multiple_cegar.h"

#include "cegar.h"
#include "pattern_database.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/countdown_timer.h"
#include "../utils/markup.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <vector>

using namespace std;

namespace pdbs {
PatternCollectionGeneratorMultipleCegar::PatternCollectionGeneratorMultipleCegar(
    options::Options &opts)
    : max_refinements(opts.get<int>("max_refinements")),
      max_pdb_size(opts.get<int>("max_pdb_size")),
      max_collection_size(opts.get<int>("max_collection_size")),
      wildcard_plans(opts.get<bool>("wildcard_plans")),
      cegar_max_time(opts.get<double>("max_time")),
      verbosity(opts.get<utils::Verbosity>("verbosity")),
      rng(utils::parse_rng_from_options(opts)),
      random_seed(opts.get<int>("random_seed")),
      stagnation_limit(opts.get<double>("stagnation_limit")),
      blacklist_trigger_time(opts.get<double>("blacklist_trigger_time")),
      blacklist_on_stagnation(opts.get<bool>("blacklist_on_stagnation")),
      total_max_time(opts.get<double>("total_max_time")) {
}

PatternCollectionInformation PatternCollectionGeneratorMultipleCegar::generate(
    const shared_ptr<AbstractTask> &task) {
    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << "Multiple CEGAR: generating patterns" << endl;
    }
    TaskProxy task_proxy(*task);

    utils::CountdownTimer timer(total_max_time);
    shared_ptr<PatternCollection> union_patterns = make_shared<PatternCollection>();
    shared_ptr<PDBCollection> union_pdbs = make_shared<PDBCollection>();
    utils::HashSet<Pattern> pattern_set; // for checking if a pattern is already in collection

    vector<FactPair> goals;
    for (FactProxy goal : task_proxy.get_goals()) {
        goals.push_back(goal.get_pair());
    }
    rng->shuffle(goals);

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

    bool can_generate = true;
    bool stagnation = false;
    // blacklisting can be forced after a period of stagnation (see stagnation_limit)
    bool force_blacklisting = false;
    double stagnation_start = 0;
    int num_iterations = 0;
    int goal_index = 0;
    int collection_size = 0;
    const utils::Verbosity cegar_verbosity(utils::Verbosity::SILENT);
    while (can_generate) {
        // we start blacklisting once a certain amount of time has passed
        // or if blacklisting was forced due to stagnation
        int blacklist_size = 0;
        if (force_blacklisting ||
            timer.get_elapsed_time() / total_max_time > blacklist_trigger_time) {
            blacklist_size = static_cast<int>(num_vars * (*rng)());
            force_blacklisting = true;
        }

        rng->shuffle(non_goal_variables);
        unordered_set<int> blacklisted_variables;
        // Select a random subset of non goals.
        for (size_t i = 0;
             i < min(static_cast<size_t>(blacklist_size), non_goal_variables.size());
             ++i) {
            int var_id = non_goal_variables[i];
//            if (verbosity >= utils::Verbosity::VERBOSE) {
//                utils::g_log << "Multiple CEGAR: blacklisting var" << var_id << endl;
//            }
            blacklisted_variables.insert(var_id);
        }

        int remaining_collection_size = max_collection_size - collection_size;
        double remaining_time = total_max_time - timer.get_elapsed_time();
        /*
          Call CEGAR with the remaining size budget (limiting one of pdb and
          collection size would be enough, but this is cleaner), with the
          remaining time limit and an RNG instance with a different random
          seed in each iteration.
        */
        auto collection_info = CEGAR(
            max_refinements,
            min(remaining_collection_size, max_collection_size),
            min(remaining_collection_size, max_collection_size),
            wildcard_plans,
            min(remaining_time, cegar_max_time),
            cegar_verbosity,
            make_shared<utils::RandomNumberGenerator>(
                random_seed + num_iterations),
            task,
            {goals[goal_index]},
            move(blacklisted_variables)).compute_pattern_collection();
        auto pattern_collection = collection_info.get_patterns();
        auto pdb_collection = collection_info.get_pdbs();
        if (pdb_collection->size() > 1) {
            cerr << "A generator computed more than one pattern" << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }

        // TODO: this is not very efficient since pattern
        // since each pattern is stored twice. Needs some optimization
        const Pattern &pattern = pattern_collection->front();
        if (!pattern_set.count(pattern)) {
            // new pattern detected, so no stagnation
            stagnation = false;
            pattern_set.insert(pattern);

            // decrease size limit
            shared_ptr<PatternDatabase> &pdb = pdb_collection->front();
            collection_size += pdb->get_size();
            if (max_collection_size - collection_size <= 0) {
                // This happens because a single CEGAR compute_pattern_collection can violate the
                // imposed size limit if already the given goal variable is
                // too large.
                if (verbosity >= utils::Verbosity::NORMAL) {
                    utils::g_log << "Multiple CEGAR: Total collection size "
                        "limit reached." << endl;
                }
                can_generate = false;
            }

            union_patterns->push_back(move(pattern));
            union_pdbs->push_back(move(pdb));
        } else {
            // no new pattern could be generated during this iteration
            // --> stagnation
            if (!stagnation) {
                stagnation = true;
                stagnation_start = timer.get_elapsed_time();
            }
        }
//        utils::g_log << "current collection size: " << collection_size << endl;

        // if stagnation has been going on for too long, then the
        // further behavior depends on the value of blacklist_on_stagnation
        if (stagnation &&
            timer.get_elapsed_time() - stagnation_start > stagnation_limit) {
            if (!blacklist_on_stagnation || force_blacklisting) {
                if (!blacklist_on_stagnation) {
                    // stagnation has been going on for too long and we are not
                    // allowed to force blacklisting, so nothing can be done.
                    if (verbosity >= utils::Verbosity::NORMAL) {
                        utils::g_log << "Multiple CEGAR: Stagnation limit "
                            "reached. Stopping generation." << endl;
                    }
                } else {
                    // stagnation in spite of blacklisting
                    if (verbosity >= utils::Verbosity::NORMAL) {
                        utils::g_log << "Multiple CEGAR: Stagnation limit "
                            "reached again. Stopping generation."
                                     << endl;
                    }
                }
                can_generate = false;
            } else {
                // We want to blacklist on stagnation but have not started
                // doing so yet.
                if (verbosity >= utils::Verbosity::NORMAL) {
                    utils::g_log << "Multiple CEGAR: Stagnation limit reached. "
                        "Forcing global blacklisting." << endl;
                }
                force_blacklisting = true;
                stagnation = false;
            }
        }

        if (timer.is_expired()) {
            if (verbosity >= utils::Verbosity::NORMAL) {
                utils::g_log << "Multiple CEGAR: time limit reached" << endl;
            }
            can_generate = false;
        }
        ++num_iterations;
        ++goal_index;
        goal_index = goal_index % goals.size();
        assert(utils::in_bounds(goal_index, goals));
    }

    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << "Multiple CEGAR: computation time: "
                     << timer.get_elapsed_time() << endl;
        utils::g_log << "Multiple CEGAR: number of iterations: "
                     << num_iterations << endl;
        utils::g_log << "Multiple CEGAR: average time per generator: "
                     << timer.get_elapsed_time() /
            static_cast<double>(num_iterations + 1)
                     << endl;
        utils::g_log << "Multiple CEGAR: final collection: " << *union_patterns
                     << endl;
        utils::g_log << "Multiple CEGAR: final collection number of patterns: "
                     << union_patterns->size() << endl;
        utils::g_log << "Multiple CEGAR: final collection summed PDB size: "
                     << collection_size << endl;
    }

    PatternCollectionInformation result(task_proxy, union_patterns);
    result.set_pdbs(union_pdbs);
    return result;
}

static shared_ptr<PatternCollectionGenerator> _parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Multiple CEGAR",
        "This pattern collection generator implements the multiple CEGAR algorithm "
        "described in the paper" + utils::format_conference_reference(
            {"Alexander Rovner", "Silvan Sievers", "Malte Helmert"},
            "Counterexample-Guided Abstraction Refinement for Pattern Selection "
            "in Optimal Classical Planning",
            "https://ai.dmi.unibas.ch/papers/rovner-et-al-icaps2019.pdf",
            "Proceedings of the 29th International Conference on Automated "
            "Planning and Scheduling (ICAPS 2019)",
            "362-367",
            "AAAI Press",
            "2019"));
    parser.add_option<double>(
        "total_max_time",
        "maximum time in seconds for the multiple CEGAR algorithm. The "
        "algorithm will always execute at least one iteration, i.e., call the "
        "CEGAR algorithm once. This limit possibly overrides the limit "
        "specified for the CEGAR algorithm.",
        "100.0",
        Bounds("0.0", "infinity"));
    parser.add_option<double>(
        "stagnation_limit",
        "maximum time in seconds the multiple CEGAR algorithm allows without "
        "generating a new pattern through the CEGAR algorithm. The multiple "
        "CEGAR algorithm terminates prematurely if this limit is hit unless "
        "blacklist_on_stagnation is enabled.",
        "20.0",
        Bounds("0.0", "infinity"));
    parser.add_option<double>(
        "blacklist_trigger_time",
        "percentage of total_max_time after which the multiple CEGAR "
        "algorithm enables blacklisting for diversification",
        "0.75",
        Bounds("0.0", "1.0"));
    parser.add_option<bool>(
        "blacklist_on_stagnation",
        "If true, the multiple CEGAR algorithm will enable blacklisting for "
        "diversification when stagnation_limit is hit for the first time and "
        "terminate when stagnation_limit is hit for the second time.",
        "true");
    add_cegar_options_to_parser(parser);
    utils::add_verbosity_option_to_parser(parser);
    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<PatternCollectionGeneratorMultipleCegar>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("multiple_cegar", _parse);
}
