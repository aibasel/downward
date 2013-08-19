#include "pattern_generation_edelkamp.h"

#include "pdb_heuristic.h"
#include "zero_one_pdbs_heuristic.h"

#include "../globals.h"
#include "../legacy_causal_graph.h"
#include "../plugin.h"
#include "../rng.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>
#include <ext/hash_set>

using namespace __gnu_cxx;
using namespace std;

PatternGenerationEdelkamp::PatternGenerationEdelkamp(const Options &opts)
    : pdb_max_size(opts.get<int>("pdb_max_size")),
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
    for (size_t i = 0; i < fitness_values.size(); ++i) {
        total_so_far += fitness_values[i];
        cumulative_fitness.push_back(total_so_far);
    }
    // total_so_far is now sum over all fitness values

    vector<vector<vector<bool> > > new_pattern_collections;
    new_pattern_collections.reserve(num_collections);
    for (size_t i = 0; i < num_collections; ++i) {
        int selected;
        if (total_so_far == 0) {
            // All fitness values are 0 => choose uniformly.
            selected = g_rng(fitness_values.size());
        } else {
            double random = g_rng() * total_so_far; // [0..total_so_far)
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
    for (size_t i = 0; i < pattern_collections.size(); ++i) {
        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            vector<bool> &pattern = pattern_collections[i][j];
            for (size_t k = 0; k < pattern.size(); ++k) {
                double random = g_rng(); // [0..1)
                if (random < mutation_probability) {
                    pattern[k].flip();
                }
            }
        }
    }
}

void PatternGenerationEdelkamp::transform_to_pattern_normal_form(const vector<bool> &bitvector,
                                                                 vector<int> &pattern) const {
    for (size_t i = 0; i < bitvector.size(); ++i) {
        if (bitvector[i])
            pattern.push_back(i);
    }
}

void PatternGenerationEdelkamp::remove_irrelevant_variables(
    vector<int> &pattern) const {
    hash_set<int> in_original_pattern(pattern.begin(), pattern.end());
    hash_set<int> in_pruned_pattern;

    vector<int> vars_to_check;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        int var_no = g_goal[i].first;
        if (in_original_pattern.count(var_no)) {
            // Goals are causally relevant.
            vars_to_check.push_back(var_no);
            in_pruned_pattern.insert(var_no);
        }
    }

    while (!vars_to_check.empty()) {
        int var = vars_to_check.back();
        vars_to_check.pop_back();
        const vector<int> &rel = g_legacy_causal_graph->get_predecessors(var);
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
}

bool PatternGenerationEdelkamp::is_pattern_too_large(
    const vector<int> &pattern) const {
    // test if the pattern respects the memory limit
    int mem = 1;
    for (size_t i = 0; i < pattern.size(); ++i) {
        int domain_size = g_variable_domain[pattern[i]];
        // test against overflow and pdb_max_size
        if (mem > pdb_max_size / domain_size)
            return true;
        mem *= domain_size;
    }
    return false;
}

bool PatternGenerationEdelkamp::mark_used_variables(
    const vector<int> &pattern, vector<bool> &variables_used) const {
    for (size_t i = 0; i < pattern.size(); ++i) {
        int var_no = pattern[i];
        if (variables_used[var_no])
            return true;
        variables_used[var_no] = true;
    }
    return false;
}

void PatternGenerationEdelkamp::evaluate(vector<double> &fitness_values) {
    for (size_t i = 0; i < pattern_collections.size(); ++i) {
        //cout << "evaluate pattern collection " << (i + 1) << " of " << pattern_collections.size() << endl;
        double fitness = 0;
        bool pattern_valid = true;
        vector<bool> variables_used(g_variable_domain.size(), false);
        vector<vector<int> > pattern_collection;
        pattern_collection.reserve(pattern_collections[i].size());
        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            const vector<bool> &bitvector = pattern_collections[i][j];
            vector<int> pattern;
            transform_to_pattern_normal_form(bitvector, pattern);

            if (is_pattern_too_large(pattern)) {
                cout << "pattern " << j << " exceeds the memory limit!" << endl;
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
            // set fitness to a very small value to cover cases in which all patterns are invalid
            fitness = 0.001;
        } else {
            // generate the pattern collection heuristic and get its fitness value.
            Options opts;
            opts.set<int>("cost_type", cost_type);
            opts.set<vector<vector<int> > >("patterns", pattern_collection);
            ZeroOnePDBsHeuristic *zoppch =
                new ZeroOnePDBsHeuristic(opts);
            fitness = zoppch->get_approx_mean_finite_h();
            // update the best heuristic found so far.
            if (fitness > best_fitness) {
                best_fitness = fitness;
                cout << "best_fitness = " << best_fitness << endl;
                delete best_heuristic;
                best_heuristic = zoppch;
                //best_heuristic->dump();
            } else {
                delete zoppch;
            }
        }
        fitness_values.push_back(fitness);
    }
}

void PatternGenerationEdelkamp::bin_packing() {
    vector<int> variables;
    variables.reserve(g_variable_domain.size());
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        variables.push_back(i);
    }

    for (size_t i = 0; i < num_collections; ++i) {
        // random variable ordering for all pattern collections
        random_shuffle(variables.begin(), variables.end(), g_rng);
        vector<vector<bool> > pattern_collection;
        vector<bool> pattern(g_variable_name.size(), false);
        size_t current_size = 1;
        for (size_t j = 0; j < variables.size(); ++j) {
            int var = variables[j];
            int next_var_size = g_variable_domain[var];
            if (next_var_size > pdb_max_size) // var never fits into a bin
                continue;
            // test against overflow and pdb_max_size
            if (current_size > pdb_max_size / next_var_size) { // open a new bin for var
                // current_size * next_var_size > pdb_max_size
                pattern_collection.push_back(pattern);
                pattern.clear();
                pattern.resize(g_variable_name.size(), false);
                current_size = 1;
            }
            current_size *= next_var_size;
            pattern[var] = true;
        }
        // the last bin has not bin inserted into pattern_collection, do so now.
        // We test current_size against 1 because this is cheaper than
        // testing if pattern is an all-zero bitvector. current_size
        // can only be 1 if *all* variables have a domain larger than
        // pdb_max_size.
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
        select(fitness_values); // we allow to select invalid pattern collections
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

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<int>("pdb_max_size", 50000, "max number of states per pdb");
    parser.add_option<int>("num_collections", 5, "number of pattern collections to maintain");
    parser.add_option<int>("num_episodes", 30, "number of episodes");
    parser.add_option<double>("mutation_probability", 0.01, "probability between 0 and 1 for flipping a bit");
    parser.add_option<bool>("disjoint", false, "using disjoint variables in the patterns of a collection");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (opts.get<int>("pdb_max_size") < 1)
        parser.error("size per pdb must be at least 1");
    if (opts.get<int>("num_collections") < 1)
        parser.error("number of pattern collections must be at least 1");
    if (opts.get<int>("num_episodes") < 0)
        parser.error("number of episodes must be a non negative number");
    if (opts.get<double>("mutation_probability") < 0 || opts.get<double>("mutation_probability") > 1)
        parser.error("mutation probability must be in [0..1]");

    if (parser.dry_run())
        return 0;
    PatternGenerationEdelkamp pge(opts);
    return pge.get_pattern_collection_heuristic();
}

static Plugin<ScalarEvaluator> _plugin("gapdb", _parse);
