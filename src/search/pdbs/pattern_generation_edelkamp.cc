#include "pattern_generation_edelkamp.h"

#include "zero_one_pdbs_heuristic.h"

#include "../causal_graph.h"
#include "../globals.h"
#include "../plugin.h"
#include "../rng.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <vector>

using namespace std;

PatternGenerationEdelkamp::PatternGenerationEdelkamp(const Options &opts)
    : task(get_task_from_options(opts)),
      task_proxy(*task),
      pdb_max_size(opts.get<int>("pdb_max_size")),
      num_collections(opts.get<int>("num_collections")),
      num_episodes(opts.get<int>("num_episodes")),
      mutation_probability(opts.get<double>("mutation_probability")),
      disjoint_patterns(opts.get<bool>("disjoint")),
      cost_type(OperatorCost(opts.get<int>("cost_type"))) {
    Timer timer;
    genetic_algorithm();
    cout << "Pattern generation (Edelkamp) time: " << timer << endl;
}

PatternGenerationEdelkamp::~PatternGenerationEdelkamp() {
}

void PatternGenerationEdelkamp::select(const vector<double> &fitness_values) {
    vector<double> cumulative_fitness;
    cumulative_fitness.reserve(fitness_values.size());
    double total_so_far = 0;
    for (double fitness_value : fitness_values) {
        total_so_far += fitness_value;
        cumulative_fitness.push_back(total_so_far);
    }
    // total_so_far is now sum over all fitness values.

    vector<vector<vector<bool> > > new_pattern_collections;
    new_pattern_collections.reserve(num_collections);
    for (int i = 0; i < num_collections; ++i) {
        int selected;
        if (total_so_far == 0) {
            // All fitness values are 0 => choose uniformly.
            selected = g_rng(fitness_values.size());
        } else {
            // [0..total_so_far)
            double random = g_rng() * total_so_far;
            // Find first entry which is strictly greater than random.
            selected = upper_bound(cumulative_fitness.begin(),
                                   cumulative_fitness.end(), random) -
                       cumulative_fitness.begin();
        }
        new_pattern_collections.push_back(pattern_collections[selected]);
    }
    pattern_collections.swap(new_pattern_collections);
}

void PatternGenerationEdelkamp::mutate() {
    for (auto &collection : pattern_collections) {
        for (vector<bool> &pattern : collection) {
            for (size_t k = 0; k < pattern.size(); ++k) {
                double random = g_rng(); // [0..1)
                if (random < mutation_probability) {
                    pattern[k].flip();
                }
            }
        }
    }
}

void PatternGenerationEdelkamp::transform_to_pattern_normal_form(
    const vector<bool> &bitvector, vector<int> &pattern) const {
    for (size_t i = 0; i < bitvector.size(); ++i) {
        if (bitvector[i])
            pattern.push_back(i);
    }
}

void PatternGenerationEdelkamp::remove_irrelevant_variables(
    vector<int> &pattern) const {
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
        const CausalGraph &cg = task_proxy.get_causal_graph();

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

bool PatternGenerationEdelkamp::is_pattern_too_large(
    const vector<int> &pattern) const {
    // Test if the pattern respects the memory limit.
    int mem = 1;
    VariablesProxy vars = task_proxy.get_variables();
    for (size_t i = 0; i < pattern.size(); ++i) {
        VariableProxy var = vars[pattern[i]];
        int domain_size = var.get_domain_size();
        if (!is_product_within_limit(mem, domain_size, pdb_max_size))
            return true;
        mem *= domain_size;
    }
    return false;
}

bool PatternGenerationEdelkamp::mark_used_variables(
    const vector<int> &pattern, vector<bool> &variables_used) const {
    for (size_t i = 0; i < pattern.size(); ++i) {
        int var_id = pattern[i];
        if (variables_used[var_id])
            return true;
        variables_used[var_id] = true;
    }
    return false;
}

void PatternGenerationEdelkamp::evaluate(vector<double> &fitness_values) {
    for (const auto &collection : pattern_collections) {
        //cout << "evaluate pattern collection " << (i + 1) << " of "
        //     << pattern_collections.size() << endl;
        double fitness = 0;
        bool pattern_valid = true;
        vector<bool> variables_used(task_proxy.get_variables().size(), false);
        vector<vector<int> > pattern_collection;
        pattern_collection.reserve(collection.size());
        for (const vector<bool> &bitvector : collection) {
            vector<int> pattern;
            transform_to_pattern_normal_form(bitvector, pattern);

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
            pattern_collection.push_back(pattern);
        }
        if (!pattern_valid) {
            /* Set fitness to a very small value to cover cases in which all
               patterns are invalid. */
            fitness = 0.001;
        } else {
            /* Generate the pattern collection heuristic and get its fitness
               value. */
            Options opts;
            opts.set<shared_ptr<AbstractTask> >("transform", task);
            // Since we pass a task transformation, cost_type won't be used.
            opts.set<int>("cost_type", NORMAL);
            opts.set<vector<vector<int> > >("patterns", pattern_collection);
            ZeroOnePDBsHeuristic *pattern_collection_heuristic =
                new ZeroOnePDBsHeuristic(opts);
            fitness = pattern_collection_heuristic->get_approx_mean_finite_h();
            // Update the best heuristic found so far.
            if (fitness > best_fitness) {
                best_fitness = fitness;
                cout << "best_fitness = " << best_fitness << endl;
                delete best_heuristic;
                best_heuristic = pattern_collection_heuristic;
                //best_heuristic->dump();
            } else {
                delete pattern_collection_heuristic;
            }
        }
        fitness_values.push_back(fitness);
    }
}

void PatternGenerationEdelkamp::bin_packing() {
    VariablesProxy variables = task_proxy.get_variables();
    vector<int> variable_ids;
    variable_ids.reserve(variables.size());
    for (size_t i = 0; i < variables.size(); ++i) {
        variable_ids.push_back(i);
    }

    for (int i = 0; i < num_collections; ++i) {
        // Use random variable ordering for all pattern collections.
        g_rng.shuffle(variable_ids);
        vector<vector<bool> > pattern_collection;
        vector<bool> pattern(variables.size(), false);
        int current_size = 1;
        for (size_t j = 0; j < variable_ids.size(); ++j) {
            int var_id = variable_ids[j];
            int next_var_size = variables[var_id].get_domain_size();
            if (next_var_size > pdb_max_size)
                // var never fits into a bin.
                continue;
            if (!is_product_within_limit(current_size, next_var_size,
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

void PatternGenerationEdelkamp::genetic_algorithm() {
    best_fitness = -1;
    best_heuristic = 0;
    bin_packing();
    //cout << "initial pattern collections:" << endl;
    //dump();
    vector<double> initial_fitness_values;
    evaluate(initial_fitness_values);
    for (int i = 0; i < num_episodes; ++i) {
        cout << endl;
        cout << "--------- episode no " << (i + 1) << " ---------" << endl;
        mutate();
        //cout << "current pattern_collections after mutation" << endl;
        //dump();
        vector<double> fitness_values;
        evaluate(fitness_values);
        // We allow to select invalid pattern collections.
        select(fitness_values);
        //cout << "current pattern collections (after selection):" << endl;
        //dump();
    }
}

void PatternGenerationEdelkamp::dump() const {
    for (size_t i = 0; i < pattern_collections.size(); ++i) {
        cout << "pattern collection no " << (i + 1) << endl;
        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            cout << pattern_collections[i][j] << endl;
        }
    }
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Genetic Algorithm PDB",
        "The following paper describes the automated creation of pattern "
        "databases with a genetic algorithm. Pattern collections are initially "
        "created with a bin-packing algorithm. The genetic algorithm is used to "
        "optimize the pattern collections with an objective function that "
        "estimates the mean heuristic value of the the pattern collections. "
        "Pattern collections with higher mean heuristic estimates are more "
        "likely selected for the next generation.\n\n"
        " * Stefan Edelkamp<<BR>>"
        " [Automated Creation of Pattern Database Search Heuristics "
        "http://www.springerlink.com/content/20613345434608x1/].<<BR>>"
        "In //Proceedings of the 4th Workshop on Model Checking and Artificial "
        "Intelligence (!MoChArt 2006)//, pp. 35-50, 2007.");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");
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

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    if (parser.dry_run())
        return 0;
    PatternGenerationEdelkamp pge(opts);
    return pge.get_pattern_collection_heuristic();
}

static Plugin<Heuristic> _plugin("gapdb", _parse);
