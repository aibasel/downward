#include "pattern_collection_generator_multiple_cegar.h"

#include "cegar.h"

#include "pattern_database.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../tasks/root_task.h"

#include "../utils/countdown_timer.h"
#include "../utils/hash.h"
#include "../utils/logging.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <vector>

using namespace std;

namespace pdbs {

PatternCollectionGeneratorMultipleCegar::PatternCollectionGeneratorMultipleCegar(
    options::Options& opts)
    : single_generator_max_refinements(opts.get<int>("max_refinements")),
      single_generator_max_pdb_size(opts.get<int>("max_pdb_size")),
      single_generator_max_collection_size(opts.get<int>("max_collection_size")),
      single_generator_wildcard_plans(opts.get<bool>("wildcard_plans")),
      single_generator_max_time(opts.get<double>("max_time")),
      single_generator_verbosity(opts.get<utils::Verbosity>("verbosity")),
      initial_random_seed(opts.get<int>("initial_random_seed")),
      total_collection_max_size(opts.get<int>("total_collection_max_size")),
      stagnation_limit(opts.get<double>("stagnation_limit")),
      blacklist_trigger_time(opts.get<double>("blacklist_trigger_time")),
      blacklist_on_stagnation(opts.get<bool>("blacklist_on_stagnation")),
      total_time_limit(opts.get<double>("total_time_limit")) {
}

PatternCollectionInformation PatternCollectionGeneratorMultipleCegar::generate(
        const std::shared_ptr<AbstractTask> &task) {
    utils::g_log << "Fast CEGAR: generating patterns" << endl;
    TaskProxy task_proxy(*task);

    utils::CountdownTimer timer(total_time_limit);
    shared_ptr<PatternCollection> union_patterns = make_shared<PatternCollection>();
    shared_ptr<PDBCollection> union_pdbs = make_shared<PDBCollection>();
    utils::HashSet<Pattern> pattern_set; // for checking if a pattern is already in collection

    size_t nvars = task_proxy.get_variables().size();
    utils::RandomNumberGenerator rng(initial_random_seed);

    vector<int> goals;
    for (auto goal : task_proxy.get_goals()) {
        int goal_var = goal.get_variable().get_id();
        goals.push_back(goal_var);
    }
    rng.shuffle(goals);

    bool can_generate = true;
    bool stagnation = false;
    // blacklisting can be forced after a period of stagnation (see stagnation_limit)
    bool force_blacklisting = false;
    double stagnation_start = 0;
    int num_iterations = 0;
    int goal_index = 0;
    const bool single_generator_ignore_goal_violations = true;
    int collection_size = 0;
    while (can_generate) {
        // we start blacklisting once a certain amount of time has passed
        // or if blacklisting was forced due to stagnation
        int blacklist_size = 0;
        if (force_blacklisting ||
                timer.get_elapsed_time() / total_time_limit > blacklist_trigger_time) {
            blacklist_size = static_cast<int>(nvars * rng());
            force_blacklisting = true;
        }

        int remaining_collection_size = total_collection_max_size - collection_size;
        double remaining_time = total_time_limit - timer.get_elapsed_time();
        auto collection_info = cegar(
            task,
            {goals[goal_index]},
            make_shared<utils::RandomNumberGenerator>(initial_random_seed + num_iterations),
            single_generator_max_refinements,
            single_generator_max_pdb_size,
            min(remaining_collection_size, single_generator_max_collection_size),
            single_generator_wildcard_plans,
            single_generator_ignore_goal_violations,
            blacklist_size,
            single_generator_verbosity,
            min(remaining_time, single_generator_max_time)
        );
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
            if (total_collection_max_size - collection_size <= 0) {
                // This happens because a single CEGAR run can violate the
                // imposed size limit if already the given goal variable is
                // too large.
                utils::g_log << "Fast CEGAR: Total collection size limit reached." << endl;
                can_generate = false;
            }

            union_patterns->push_back(std::move(pattern));
            union_pdbs->push_back(std::move(pdb));
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
                    utils::g_log << "Fast CEGAR: Stagnation limit reached. Stopping generation."
                         << endl;
                } else {
                    // stagnation in spite of blacklisting
                    utils::g_log << "Fast CEGAR: Stagnation limit reached again. Stopping generation."
                         << endl;
                }
                can_generate = false;
            } else {
                // We want to blacklist on stagnation but have not started
                // doing so yet.
                utils::g_log << "Fast CEGAR: Stagnation limit reached. Forcing global blacklisting."
                     << endl;
                force_blacklisting = true;
                stagnation = false;
            }
        }

        if (timer.is_expired()) {
            utils::g_log << "Fast CEGAR: time limit reached" << endl;
            can_generate = false;
        }
        ++num_iterations;
        ++goal_index;
        goal_index = goal_index % goals.size();
        assert(utils::in_bounds(goal_index, goals));
    }

    utils::g_log << "Fast CEGAR: computation time: " << timer.get_elapsed_time() << endl;
    utils::g_log << "Fast CEGAR: number of iterations: " << num_iterations << endl;
    utils::g_log << "Fast CEGAR: average time per generator: "
         << timer.get_elapsed_time() / static_cast<double>(num_iterations + 1)
         << endl;
    utils::g_log << "Fast CEGAR: final collection: " << *union_patterns << endl;
    utils::g_log << "Fast CEGAR: final collection number of patterns: " << union_patterns->size() << endl;
    utils::g_log << "Fast CEGAR: final collection summed PDB size: " << collection_size << endl;

    PatternCollectionInformation result(task_proxy,union_patterns);
    result.set_pdbs(union_pdbs);
    return result;
}

static shared_ptr<PatternCollectionGenerator> _parse(options::OptionParser &parser) {
    parser.add_option<int>(
            "initial_random_seed",
            "seed for the random number generator(s) of the cegar generators","10");
    parser.add_option<int>(
            "total_collection_max_size",
            "max. number of entries in the final collection",
            "infinity"
    );
    parser.add_option<double>(
            "total_time_limit",
            "time budget for PDB collection generation",
            "25.0"
    );
    parser.add_option<double>(
            "stagnation_limit",
            "max. time the algorithm waits for the introduction of a new pattern."
                    " Execution finishes prematurely if no new, unique pattern"
                    " could be added to the collection during this time.",
            "5.0"
    );
    parser.add_option<double>(
            "blacklist_trigger_time",
            "time given as percentage of overall time_limit,"
                    " after which blacklisting (for diversification) is enabled."
                    " E.g. blacklist_trigger_time=0.5 will trigger blacklisting"
                    " once half of the total time has passed.",
            "1.0",
            Bounds("0.0","1.0")
    );
    parser.add_option<bool>(
            "blacklist_on_stagnation",
            "whether the algorithm forces blacklisting to start early if"
                    " stagnation_limit is crossed (instead of aborting)."
                    " The algorithm still aborts if stagnation_limit is"
                    " reached for the second time.",
            "true"
    );

    add_cegar_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<PatternCollectionGeneratorMultipleCegar>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("multiple_cegar", _parse);
}
