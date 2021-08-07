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
    options::Options &opts, const string &name)
    : name(name),
      max_pdb_size(opts.get<int>("max_pdb_size")),
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
      time_point_of_last_new_pattern(0.0) {
}

void PatternCollectionGeneratorMultiple::check_blacklist_trigger_timer(
    const utils::CountdownTimer &timer) {
    // Check if blacklisting should be started.
    if (!blacklisting && timer.get_elapsed_time() > blacklisting_start_time) {
        blacklisting = true;
        /*
          Also treat this time point as having seen a new pattern to avoid
          stopping due to stagnation right after enabling blacklisting.
        */
        time_point_of_last_new_pattern = timer.get_elapsed_time();
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
        int blacklist_size = rng->random(non_goal_variables.size());
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
        /*
          compute_pattern generated a new pattern. Create/retrieve corresponding
          PDB, update collection size and reset time_point_of_last_new_pattern.
        */
        time_point_of_last_new_pattern = timer.get_elapsed_time();
        shared_ptr<PatternDatabase> pdb = pattern_info.get_pdb();
        remaining_collection_size -= pdb->get_size();
        generated_pdbs->push_back(move(pdb));
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
    if (timer.get_elapsed_time() - time_point_of_last_new_pattern > stagnation_limit) {
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
                time_point_of_last_new_pattern = timer.get_elapsed_time();
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
        utils::g_log << "Generating patterns using the " << name
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
        utils::g_log << name << " number of iterations: "
                     << num_iterations << endl;
        utils::g_log << name << " average time per generator: "
                     << timer.get_elapsed_time() / num_iterations
                     << endl;
        dump_pattern_collection_generation_statistics(
            name,
            timer.get_elapsed_time(),
            result);
    }
    return result;
}

void add_multiple_algorithm_implementation_notes_to_parser(
    options::OptionParser &parser) {
    parser.document_note(
        "Short description of the 'multiple algorithm framework'",
        "This algorithm is a general framework for computing a pattern collection "
        "for a given planning task. It requires as input a method for computing a "
        "single pattern for the given task and a single goal of the task. The "
        "algorithm works as follows. It first stores the goals of the task in "
        "random order. Then, it repeatedly iterates over all goals and for each "
        "goal, it uses the given method for computing a single pattern. If the "
        "pattern is new (duplicate detection), it is kept for the final collection.\n"
        "The algorithm runs until reaching a given time limit. Another parameter allows "
        "exiting early if no new patterns are found for a certain time ('stagnation'). "
        "Further parameters allow enabling blacklisting for the given pattern computation "
        "method after a certain time to force some diversification or to enable said "
        "blacklisting when stagnating.",
        true);
    parser.document_note(
        "Implementation note about the 'multiple algorithm framework'",
        "A difference compared to the original implementation used in the "
        "paper is that the original implementation of stagnation in "
        "the multiple CEGAR/RCG algorithms started counting the time towards "
        "stagnation only after having generated a duplicate pattern. Now, "
        "time towards stagnation starts counting from the start and is reset "
        "to the current time only when having found a new pattern or when "
        "enabling blacklisting.",
        true);
}

void add_multiple_options_to_parser(options::OptionParser &parser) {
    parser.add_option<int>(
        "max_pdb_size",
        "maximum number of states for each pattern database, computed "
        "by compute_pattern (possibly ignored by singleton patterns consisting "
        "of a goal variable)",
        "1M",
        Bounds("1", "infinity"));
    parser.add_option<int>(
        "max_collection_size",
        "maximum number of states in all pattern databases of the "
        "collection (possibly ignored, see max_pdb_size)",
        "10M",
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
