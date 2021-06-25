#include "pattern_collection_generator_multiple.h"

#include "pattern_database.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/countdown_timer.h"
#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <vector>

using namespace std;

namespace pdbs {
PatternCollectionGeneratorMultiple::PatternCollectionGeneratorMultiple(
    options::Options &opts)
    : max_pdb_size(opts.get<int>("max_pdb_size")),
      pattern_generation_max_time(opts.get<double>("pattern_generation_max_time")),
      total_max_time(opts.get<double>("total_max_time")),
      stagnation_limit(opts.get<double>("stagnation_limit")),
      blacklisting_start_time(total_max_time * opts.get<double>("blacklist_trigger_percentage")),
      enable_blacklist_on_stagnation(opts.get<bool>("enable_blacklist_on_stagnation")),
      verbosity(opts.get<utils::Verbosity>("verbosity")),
      rng(utils::parse_rng_from_options(opts)),
      random_seed(opts.get<int>("random_seed")),
      remaining_collection_size(opts.get<int>("max_collection_size")),
      blacklisting(false),
      stagnation_start_time(-1) {
}

void PatternCollectionGeneratorMultiple::check_blacklist_trigger_timer(
    const utils::CountdownTimer &timer) {
    // Check if blacklisting should be started.
    if (!blacklisting && timer.get_elapsed_time() > blacklisting_start_time) {
        blacklisting = true;
        // Also reset stagnation timer in case it was already set.
        stagnation_start_time = -1;
        if (verbosity >= utils::Verbosity::NORMAL) {
            utils::g_log << "given percentage of total time limit "
                         << "exhausted; enabling blacklisting." << endl;
        }
    }
}

unordered_set<int> PatternCollectionGeneratorMultiple::get_blacklisted_variables(
    vector<int> &non_goal_variables) {
    unordered_set<int> blacklisted_variables;
    if (blacklisting && !non_goal_variables.empty()) {
        /*
          Randomize the number of non-goal variables for blacklisting.
          We want to choose at least 1 non-goal variable, so we pick a random
          value in the range [1, |non-goal variables|].
        */
        int blacklist_size = (*rng)(non_goal_variables.size());
        ++blacklist_size;
        rng->shuffle(non_goal_variables);
        blacklisted_variables.insert(
            non_goal_variables.begin(), non_goal_variables.begin() + blacklist_size);
        if (verbosity >= utils::Verbosity::DEBUG) {
            utils::g_log << "blacklisting " << blacklist_size << " out of "
                         << non_goal_variables.size()
                         << " non-goal variables: ";
            for (int var : blacklisted_variables) {
                utils::g_log << var << ", ";
            }
            utils::g_log << endl;
        }
    }
    return blacklisted_variables;
}

void PatternCollectionGeneratorMultiple::handle_generated_pattern(
    PatternInformation &&pattern_info,
    set<Pattern> &generated_patterns,
    shared_ptr<PDBCollection> &generated_pdbs,
    const utils::CountdownTimer &timer) {
    const Pattern &pattern = pattern_info.get_pattern();
    if (verbosity >= utils::Verbosity::DEBUG) {
        utils::g_log << "generated pattern " << pattern << endl;
    }
    if (generated_patterns.insert(move(pattern)).second) {
        // compute_pattern generated a new pattern. Reset stagnation_start_time.
        stagnation_start_time = -1;
        shared_ptr<PatternDatabase> pdb = pattern_info.get_pdb();
        remaining_collection_size -= pdb->get_size();
        generated_pdbs->push_back(move(pdb));
    } else {
        // Pattern is not new. Set stagnation start time if not already set.
        if (stagnation_start_time == -1) {
            stagnation_start_time = timer.get_elapsed_time();
        }
    }
}

bool PatternCollectionGeneratorMultiple::collection_size_limit_reached() const {
    if (remaining_collection_size <= 0) {
        /*
          This value can become negative if the given size limits for
          pdb or collection size are so low that compute_pattern already
          violates the limit, possibly even with only using a single goal
          variable.
        */
        if (verbosity >= utils::Verbosity::NORMAL) {
            utils::g_log << "collection size limit reached" << endl;
        }
        return true;
    }
    return false;
}

bool PatternCollectionGeneratorMultiple::time_limit_reached(
    const utils::CountdownTimer &timer) const {
    if (timer.is_expired()) {
        if (verbosity >= utils::Verbosity::NORMAL) {
            utils::g_log << "time limit reached" << endl;
        }
        return true;
    }
    return false;
}

bool PatternCollectionGeneratorMultiple::check_for_stagnation(
    const utils::CountdownTimer &timer) {
    // Test if no new pattern was generated for longer than stagnation_limit.
    if (stagnation_start_time != -1 &&
        timer.get_elapsed_time() - stagnation_start_time > stagnation_limit) {
        if (enable_blacklist_on_stagnation) {
            if (blacklisting) {
                if (verbosity >= utils::Verbosity::NORMAL) {
                    utils::g_log << "stagnation limit reached "
                                 << "despite blacklisting, terminating"
                                 << endl;
                }
                return true;
            } else {
                if (verbosity >= utils::Verbosity::NORMAL) {
                    utils::g_log << "stagnation limit reached, "
                                 << "enabling blacklisting" << endl;
                }
                blacklisting = true;
                stagnation_start_time = -1;
            }
        } else {
            if (verbosity >= utils::Verbosity::NORMAL) {
                utils::g_log << "stagnation limit reached, terminating" << endl;
            }
            return true;
        }
    }
    return false;
}

PatternCollectionInformation PatternCollectionGeneratorMultiple::generate(
    const shared_ptr<AbstractTask> &task) {
    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << "Generating patterns using the " << get_name()
                     << " algorithm." << endl;
        utils::g_log << "max pdb size: " << max_pdb_size << endl;
        utils::g_log << "max collection size: " << remaining_collection_size << endl;
        utils::g_log << "max time: " << total_max_time << endl;
        utils::g_log << "stagnation time limit: " << stagnation_limit << endl;
        utils::g_log << "timer after which blacklisting is enabled: "
                     << blacklisting_start_time << endl;
        utils::g_log << "enable blacklisting after stagnation: "
                     << enable_blacklist_on_stagnation << endl;
        utils::g_log << "verbosity: ";
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
    }

    TaskProxy task_proxy(*task);
    utils::CountdownTimer timer(total_max_time);

    // Store the set of goals in random order.
    vector<FactPair> goals = get_goals_in_random_order(task_proxy, *rng);

    // Store the non-goal variables for potential blacklisting.
    vector<int> non_goal_variables = get_non_goal_variables(task_proxy);

    if (verbosity >= utils::Verbosity::DEBUG) {
        utils::g_log << "goal variables: ";
        for (FactPair goal : goals) {
            utils::g_log << goal.var << ", ";
        }
        utils::g_log << endl;
        utils::g_log << "non-goal variables: " << non_goal_variables << endl;
    }

    initialize(task);

    // Collect all unique patterns and their PDBs.
    set<Pattern> generated_patterns;
    shared_ptr<PDBCollection> generated_pdbs = make_shared<PDBCollection>();

    shared_ptr<utils::RandomNumberGenerator> pattern_computation_rng =
        make_shared<utils::RandomNumberGenerator>(random_seed);
    int num_iterations = 1;
    int goal_index = 0;
    while (true) {
        check_blacklist_trigger_timer(timer);

        unordered_set<int> blacklisted_variables =
            get_blacklisted_variables(non_goal_variables);

        int remaining_pdb_size = min(remaining_collection_size, max_pdb_size);
        double remaining_time =
            min(static_cast<double>(timer.get_remaining_time()), pattern_generation_max_time);

        PatternInformation pattern_info = compute_pattern(
            remaining_pdb_size,
            remaining_time,
            pattern_computation_rng,
            task,
            goals[goal_index],
            move(blacklisted_variables));
        handle_generated_pattern(
            move(pattern_info),
            generated_patterns,
            generated_pdbs,
            timer);

        if (collection_size_limit_reached() ||
            time_limit_reached(timer) ||
            check_for_stagnation(timer)) {
            break;
        }

        ++num_iterations;
        ++goal_index;
        goal_index = goal_index % goals.size();
        assert(utils::in_bounds(goal_index, goals));
    }

    PatternCollectionInformation result = get_pattern_collection_info(
        task_proxy, generated_pdbs);
    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << get_name() << " number of iterations: "
                     << num_iterations << endl;
        utils::g_log << get_name() << " average time per generator: "
                     << timer.get_elapsed_time() / num_iterations
                     << endl;
        dump_pattern_collection_generation_statistics(
            get_name(),
            timer.get_elapsed_time(),
            result);
    }
    return result;
}

void add_multiple_options_to_parser(options::OptionParser &parser) {
    parser.add_option<int>(
        "max_pdb_size",
        "maximum number of states for each pattern database, computed "
        "by compute_pattern (possibly ignored by singleton patterns consisting "
        "of a goal variable)",
        "1000000",
        Bounds("1", "infinity"));
    parser.add_option<int>(
        "max_collection_size",
        "maximum number of states in all pattern databases of the "
        "collection (possibly ignored, see max_pdb_size)",
        "10000000",
        Bounds("1", "infinity"));
    parser.add_option<double>(
        "pattern_generation_max_time",
        "maximum time in seconds for each call to the algorithm for "
        "computing a single pattern",
        "infinity",
        Bounds("0.0", "infinity"));
    parser.add_option<double>(
        "total_max_time",
        "maximum time in seconds for this pattern collection generator. "
        "It will always execute at least one iteration, i.e., call the "
        "algorithm for computing a single pattern at least once.",
        "100.0",
        Bounds("0.0", "infinity"));
    parser.add_option<double>(
        "stagnation_limit",
        "maximum time in seconds this pattern generator is allowed to run "
        "without generating a new pattern. It terminates prematurely if this "
        "limit is hit unless enable_blacklist_on_stagnation is enabled.",
        "20.0",
        Bounds("1.0", "infinity"));
    parser.add_option<double>(
        "blacklist_trigger_percentage",
        "percentage of total_max_time after which blacklisting is enabled",
        "0.75",
        Bounds("0.0", "1.0"));
    parser.add_option<bool>(
        "enable_blacklist_on_stagnation",
        "if true, blacklisting is enabled when stagnation_limit is hit "
        "for the first time (unless it was already enabled due to "
        "blacklist_trigger_percentage) and pattern generation is terminated "
        "when stagnation_limit is hit for the second time. If false, pattern "
        "generation is terminated already the first time stagnation_limit is "
        "hit.",
        "true");
    utils::add_verbosity_option_to_parser(parser);
    utils::add_rng_options(parser);
}
}
