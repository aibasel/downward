#include "pattern_collection_generator_rcg.h"

#include "pattern_database.h"
#include "rcg.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../task_utils/causal_graph.h"

#include "../utils/countdown_timer.h"
#include "../utils/hash.h"
#include "../utils/logging.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <vector>

using namespace std;

namespace pdbs {

PatternCollectionGeneratorRCG::PatternCollectionGeneratorRCG(
    options::Options& opts)
    : single_generator_max_pdb_size(opts.get<int>("max_pdb_size")),
      initial_random_seed(opts.get<int>("initial_random_seed")),
      total_collection_max_size(opts.get<int>("total_collection_max_size")),
      stagnation_limit(opts.get<double>("stagnation_limit")),
      total_time_limit(opts.get<double>("total_time_limit")),
      disable_output(opts.get<bool>("disable_stdout")),
      bidirectional(opts.get<bool>("bidirectional")),
      max_num_iterations(opts.get<int>("max_num_iterations")) {
}

size_t get_pdb_size(const TaskProxy &tp, const Pattern &P) {
    size_t num_states = 1;

    for (auto vid : P) {
        VariableProxy v = tp.get_variables()[vid];
        num_states *= v.get_domain_size();
    }

    return num_states;
}

PatternCollectionInformation PatternCollectionGeneratorRCG::generate(
        const std::shared_ptr<AbstractTask> &task) {
    utils::g_log << "SPS: generating patterns" << endl;
    TaskProxy task_proxy(*task);
    int num_vars = task_proxy.get_variables().size();
    const causal_graph::CausalGraph &cg = causal_graph::get_causal_graph(task.get());

    // Compute the CG neighbors once to allow RCG shuffling them.
    vector<vector<int>> cg_neighbors(num_vars);
    for (int var_id = 0; var_id < num_vars; ++var_id) {
        cg_neighbors[var_id] = cg.get_predecessors(var_id);
        if (bidirectional) {
            const vector<int> &successors = cg.get_successors(var_id);
            cg_neighbors[var_id].insert(cg_neighbors[var_id].end(), successors.begin(), successors.end());
        }
        utils::sort_unique(cg_neighbors[var_id]);
    }

    utils::CountdownTimer timer(total_time_limit);
    shared_ptr<PatternCollection> union_patterns = make_shared<PatternCollection>();
    utils::HashSet<Pattern> pattern_set; // for checking if a pattern is already in collection

    shared_ptr<utils::RandomNumberGenerator> rng
        = make_shared<utils::RandomNumberGenerator>(initial_random_seed);

    vector<int> goals;
    for (auto goal : task_proxy.get_goals()) {
        int goal_var = goal.get_variable().get_id();
        goals.push_back(goal_var);
    }
    rng->shuffle(goals);

    bool can_generate = true;
    bool stagnation = false;
    double stagnation_start = 0;
    int num_iterations = 0;
    int goal_index = 0;
    shared_ptr<PDBCollection> pdbs = make_shared<PDBCollection>();
    int collection_size = 0;
    while (can_generate && num_iterations< max_num_iterations) {
        Pattern P = generate_pattern(
            min(single_generator_max_pdb_size, total_collection_max_size - collection_size),
            goals[goal_index],
            make_shared<utils::RandomNumberGenerator>(initial_random_seed + num_iterations),
            task_proxy,
            cg_neighbors
        );

        // TODO: this is not very efficient since pattern
        // since each pattern is stored twice. Needs some optimization
        if (!pattern_set.count(P)) {
            // new pattern detected, so no stagnation
            stagnation = false;
            pattern_set.insert(P);

            // add newly created pattern to collection and compute the PDB
            pdbs->push_back(make_shared<PatternDatabase>(task_proxy, P));
            union_patterns->push_back(std::move(P));
            collection_size += pdbs->back()->get_size();

            if (total_collection_max_size - collection_size <= 0) {
                utils::g_log << "SPS: Total collection size limit reached." << endl;
                can_generate = false;
            }
        } else {
            // no new pattern could be generated during this iteration
            // --> stagnation
            if (!stagnation) {
                stagnation = true;
                stagnation_start = timer.get_elapsed_time();
            }
        }

        if (stagnation &&
            timer.get_elapsed_time() - stagnation_start > stagnation_limit) {
            utils::g_log << "SPS: Stagnation limit reached. Stopping generation." << endl;
            can_generate = false;
        }

        if (timer.is_expired()) {
            utils::g_log << "SPS: time limit reached" << endl;
            can_generate = false;
        }
        ++num_iterations;
        ++goal_index;
        goal_index = goal_index % goals.size();
        assert(utils::in_bounds(goal_index, goals));
    }

    utils::g_log << "SPS: computation time: " << timer.get_elapsed_time() << endl;
    utils::g_log << "SPS: number of iterations: " << num_iterations << endl;
    utils::g_log << "SPS: average time per generator: "
         << timer.get_elapsed_time() / static_cast<double>(num_iterations + 1)
         << endl;
    utils::g_log << "SPS: final collection: " << *union_patterns << endl;
    utils::g_log << "SPS: final collection number of patterns: " << union_patterns->size() << endl;
    utils::g_log << "SPS: final collection summed PDB size: " << collection_size << endl;
    PatternCollectionInformation result(task_proxy,union_patterns);
    result.set_pdbs(pdbs);
    return result;
}

static shared_ptr<PatternCollectionGenerator> _parse(options::OptionParser &parser) {
    parser.add_option<int>(
            "max_pdb_size",
            "maximum allowed number of states in a pdb",
            "1000000"
    );
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
            "3.0"
    );
    parser.add_option<double>(
            "stagnation_limit",
            "max. time the algorithm waits for the introduction of a new pattern."
                    " Execution finishes prematurely if no new, unique pattern"
                    " could be added to the collection during this time.",
            "5.0"
    );
    parser.add_option<bool>(
            "disable_stdout",
            "disables standard output for this generator,"
                    "to avoid hitting the hard limit for log sizes",
            "false"
    );
    parser.add_option<bool>(
            "bidirectional",
            "consider pre-eff arcs in both directions",
            "true"
            );
    parser.add_option<int>(
            "max_num_iterations",
            "Maximum number of iterations.",
            "infinity"
            );

    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<PatternCollectionGeneratorRCG>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("mrcg", _parse);
}
