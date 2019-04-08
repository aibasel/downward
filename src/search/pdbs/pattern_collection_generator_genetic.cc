#include "pattern_collection_generator_genetic.h"

#include "utils.h"
#include "validation.h"
#include "zero_one_pdbs.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../task_utils/causal_graph.h"
#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/math.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_set>
#include <vector>

using namespace std;

namespace pdbs {
PatternCollectionGeneratorGenetic::PatternCollectionGeneratorGenetic(
    const Options &opts)
    : pdb_max_size(opts.get<int>("pdb_max_size")),
      num_collections(opts.get<int>("num_collections")),
      num_episodes(opts.get<int>("num_episodes")),
      mutation_probability(opts.get<double>("mutation_probability")),
      disjoint_patterns(opts.get<bool>("disjoint")),
      rng(utils::parse_rng_from_options(opts)) {
}

void PatternCollectionGeneratorGenetic::select(
    const vector<double> &fitness_values) {
    vector<double> cumulative_fitness;
    cumulative_fitness.reserve(fitness_values.size());
    double total_so_far = 0;
    for (double fitness_value : fitness_values) {
        total_so_far += fitness_value;
        cumulative_fitness.push_back(total_so_far);
    }
    // total_so_far is now sum over all fitness values.

    vector<vector<vector<bool>>> new_pattern_collections;
    new_pattern_collections.reserve(num_collections);
    for (int i = 0; i < num_collections; ++i) {
        int selected;
        if (total_so_far == 0) {
            // All fitness values are 0 => choose uniformly.
            selected = (*rng)(fitness_values.size());
        } else {
            // [0..total_so_far)
            double random = (*rng)() * total_so_far;
            // Find first entry which is strictly greater than random.
            selected = upper_bound(cumulative_fitness.begin(),
                                   cumulative_fitness.end(), random) -
                cumulative_fitness.begin();
        }
        new_pattern_collections.push_back(pattern_collections[selected]);
    }
    pattern_collections.swap(new_pattern_collections);
}

void PatternCollectionGeneratorGenetic::mutate() {
    for (auto &collection : pattern_collections) {
        for (vector<bool> &pattern : collection) {
            for (size_t k = 0; k < pattern.size(); ++k) {
                double random = (*rng)(); // [0..1)
                if (random < mutation_probability) {
                    pattern[k].flip();
                }
            }
        }
    }
}

Pattern PatternCollectionGeneratorGenetic::transform_to_pattern_normal_form(
    const vector<bool> &bitvector) const {
    Pattern pattern;
    for (size_t i = 0; i < bitvector.size(); ++i) {
        if (bitvector[i])
            pattern.push_back(i);
    }
    return pattern;
}

void PatternCollectionGeneratorGenetic::remove_irrelevant_variables(
    Pattern &pattern) const {
    TaskProxy task_proxy(*task);

    unordered_set<int> in_original_pattern(pattern.begin(), pattern.end());
    unordered_set<int> in_pruned_pattern;

    vector<int> vars_to_check;
    for (FactProxy goal : task_proxy.get_goals()) {
        int var_id = goal.get_variable().get_id();
        if (in_original_pattern.count(var_id)) {
            // Goals are causally relevant.
            vars_to_check.push_back(var_id);
            in_pruned_pattern.insert(var_id);
        }
    }

    while (!vars_to_check.empty()) {
        int var = vars_to_check.back();
        vars_to_check.pop_back();
        /*
          A variable is relevant to the pattern if it is a goal variable or if
          there is a pre->eff arc from the variable to a relevant variable.
          Note that there is no point in considering eff->eff arcs here.
        */
        const causal_graph::CausalGraph &cg = task_proxy.get_causal_graph();

        const vector<int> &rel = cg.get_eff_to_pre(var);
        for (size_t i = 0; i < rel.size(); ++i) {
            int var_no = rel[i];
            if (in_original_pattern.count(var_no) &&
                !in_pruned_pattern.count(var_no)) {
                // Parents of relevant variables are causally relevant.
                vars_to_check.push_back(var_no);
                in_pruned_pattern.insert(var_no);
            }
        }
    }

    pattern.assign(in_pruned_pattern.begin(), in_pruned_pattern.end());
    sort(pattern.begin(), pattern.end());
}

bool PatternCollectionGeneratorGenetic::is_pattern_too_large(
    const Pattern &pattern) const {
    // Test if the pattern respects the memory limit.
    TaskProxy task_proxy(*task);
    VariablesProxy variables = task_proxy.get_variables();
    int mem = 1;
    for (size_t i = 0; i < pattern.size(); ++i) {
        VariableProxy var = variables[pattern[i]];
        int domain_size = var.get_domain_size();
        if (!utils::is_product_within_limit(mem, domain_size, pdb_max_size))
            return true;
        mem *= domain_size;
    }
    return false;
}

bool PatternCollectionGeneratorGenetic::mark_used_variables(
    const Pattern &pattern, vector<bool> &variables_used) const {
    for (size_t i = 0; i < pattern.size(); ++i) {
        int var_id = pattern[i];
        if (variables_used[var_id])
            return true;
        variables_used[var_id] = true;
    }
    return false;
}

void PatternCollectionGeneratorGenetic::evaluate(vector<double> &fitness_values) {
    TaskProxy task_proxy(*task);
    for (const auto &collection : pattern_collections) {
        //cout << "evaluate pattern collection " << (i + 1) << " of "
        //     << pattern_collections.size() << endl;
        double fitness = 0;
        bool pattern_valid = true;
        vector<bool> variables_used(task_proxy.get_variables().size(), false);
        shared_ptr<PatternCollection> pattern_collection = make_shared<PatternCollection>();
        pattern_collection->reserve(collection.size());
        for (const vector<bool> &bitvector : collection) {
            Pattern pattern = transform_to_pattern_normal_form(bitvector);

            if (is_pattern_too_large(pattern)) {
                cout << "pattern exceeds the memory limit!" << endl;
                pattern_valid = false;
                break;
            }

            if (disjoint_patterns) {
                if (mark_used_variables(pattern, variables_used)) {
                    cout << "patterns are not disjoint anymore!" << endl;
                    pattern_valid = false;
                    break;
                }
            }

            remove_irrelevant_variables(pattern);
            pattern_collection->push_back(pattern);
        }
        if (!pattern_valid) {
            /* Set fitness to a very small value to cover cases in which all
               patterns are invalid. */
            fitness = 0.001;
        } else {
            /* Generate the pattern collection heuristic and get its fitness
               value. */
            ZeroOnePDBs zero_one_pdbs(task_proxy, *pattern_collection);
            fitness = zero_one_pdbs.compute_approx_mean_finite_h();
            // Update the best heuristic found so far.
            if (fitness > best_fitness) {
                best_fitness = fitness;
                cout << "best_fitness = " << best_fitness << endl;
                best_patterns = pattern_collection;
            }
        }
        fitness_values.push_back(fitness);
    }
}

void PatternCollectionGeneratorGenetic::bin_packing() {
    TaskProxy task_proxy(*task);
    VariablesProxy variables = task_proxy.get_variables();

    vector<int> variable_ids;
    variable_ids.reserve(variables.size());
    for (size_t i = 0; i < variables.size(); ++i) {
        variable_ids.push_back(i);
    }

    for (int i = 0; i < num_collections; ++i) {
        // Use random variable ordering for all pattern collections.
        rng->shuffle(variable_ids);
        vector<vector<bool>> pattern_collection;
        vector<bool> pattern(variables.size(), false);
        int current_size = 1;
        for (size_t j = 0; j < variable_ids.size(); ++j) {
            int var_id = variable_ids[j];
            int next_var_size = variables[var_id].get_domain_size();
            if (next_var_size > pdb_max_size)
                // var never fits into a bin.
                continue;
            if (!utils::is_product_within_limit(current_size, next_var_size,
                                                pdb_max_size)) {
                // Open a new bin for var.
                pattern_collection.push_back(pattern);
                pattern.clear();
                pattern.resize(variables.size(), false);
                current_size = 1;
            }
            current_size *= next_var_size;
            pattern[var_id] = true;
        }
        /*
          The last bin has not bin inserted into pattern_collection, do so now.
          We test current_size against 1 because this is cheaper than
          testing if pattern is an all-zero bitvector. current_size
          can only be 1 if *all* variables have a domain larger than
          pdb_max_size.
        */
        if (current_size > 1) {
            pattern_collection.push_back(pattern);
        }
        pattern_collections.push_back(pattern_collection);
    }
}

void PatternCollectionGeneratorGenetic::genetic_algorithm() {
    best_fitness = -1;
    best_patterns = nullptr;
    bin_packing();
    vector<double> initial_fitness_values;
    evaluate(initial_fitness_values);
    for (int i = 0; i < num_episodes; ++i) {
        cout << endl;
        cout << "--------- episode no " << (i + 1) << " ---------" << endl;
        mutate();
        vector<double> fitness_values;
        evaluate(fitness_values);
        // We allow to select invalid pattern collections.
        select(fitness_values);
    }
}

PatternCollectionInformation PatternCollectionGeneratorGenetic::generate(
    const shared_ptr<AbstractTask> &task_) {
    utils::Timer timer;
    cout << "Generating patterns using the genetic generator..." << endl;
    task = task_;
    genetic_algorithm();

    TaskProxy task_proxy(*task);
    assert(best_patterns);
    PatternCollectionInformation pci(task_proxy, best_patterns);
    dump_pattern_collection_generation_statistics(
        "Genetic generator", timer(), pci);
    return pci;
}

static shared_ptr<PatternCollectionGenerator> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Genetic Algorithm Patterns",
        "The following paper describes the automated creation of pattern "
        "databases with a genetic algorithm. Pattern collections are initially "
        "created with a bin-packing algorithm. The genetic algorithm is used "
        "to optimize the pattern collections with an objective function that "
        "estimates the mean heuristic value of the the pattern collections. "
        "Pattern collections with higher mean heuristic estimates are more "
        "likely selected for the next generation." + utils::format_conference_reference(
            {"Stefan Edelkamp"},
            "Automated Creation of Pattern Database Search Heuristics",
            "http://www.springerlink.com/content/20613345434608x1/",
            "Proceedings of the 4th Workshop on Model Checking and Artificial"
            " Intelligence (!MoChArt 2006)",
            "35-50",
            "AAAI Press",
            "2007"));
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_note(
        "Note",
        "This pattern generation method uses the "
        "zero/one pattern database heuristic.");
    parser.document_note(
        "Implementation Notes",
        "The standard genetic algorithm procedure as described in the paper is "
        "implemented in Fast Downward. The implementation is close to the "
        "paper.\n\n"
        "+ Initialization<<BR>>"
        "In Fast Downward bin-packing with the next-fit strategy is used. A "
        "bin corresponds to a pattern which contains variables up to "
        "``pdb_max_size``. With this method each variable occurs exactly in "
        "one pattern of a collection. There are ``num_collections`` "
        "collections created.\n"
        "+ Mutation<<BR>>"
        "With probability ``mutation_probability`` a bit is flipped meaning "
        "that either a variable is added to a pattern or deleted from a "
        "pattern.\n"
        "+ Recombination<<BR>>"
        "Recombination isn't implemented in Fast Downward. In the paper "
        "recombination is described but not used.\n"
        "+ Evaluation<<BR>>"
        "For each pattern collection the mean heuristic value is computed. For "
        "a single pattern database the mean heuristic value is the sum of all "
        "pattern database entries divided through the number of entries. "
        "Entries with infinite heuristic values are ignored in this "
        "calculation. The sum of these individual mean heuristic values yield "
        "the mean heuristic value of the collection.\n"
        "+ Selection<<BR>>"
        "The higher the mean heuristic value of a pattern collection is, the "
        "more likely this pattern collection should be selected for the next "
        "generation. Therefore the mean heuristic values are normalized and "
        "converted into probabilities and Roulette Wheel Selection is used.\n"
        "+\n\n", true);

    parser.add_option<int>(
        "pdb_max_size",
        "maximal number of states per pattern database ",
        "50000",
        Bounds("1", "infinity"));
    parser.add_option<int>(
        "num_collections",
        "number of pattern collections to maintain in the genetic "
        "algorithm (population size)",
        "5",
        Bounds("1", "infinity"));
    parser.add_option<int>(
        "num_episodes",
        "number of episodes for the genetic algorithm",
        "30",
        Bounds("0", "infinity"));
    parser.add_option<double>(
        "mutation_probability",
        "probability for flipping a bit in the genetic algorithm",
        "0.01",
        Bounds("0.0", "1.0"));
    parser.add_option<bool>(
        "disjoint",
        "consider a pattern collection invalid (giving it very low "
        "fitness) if its patterns are not disjoint",
        "false");

    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternCollectionGeneratorGenetic>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("genetic", _parse);
}
