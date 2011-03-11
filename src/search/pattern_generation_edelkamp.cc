#include "pattern_generation_edelkamp.h"
#include "globals.h"
#include "pdb_collection_heuristic.h"
#include "pdb_heuristic.h"
#include "plugin.h"
#include "rng.h"
#include "timer.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>

namespace std { using namespace __gnu_cxx; }

using namespace std;

static ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run);
static ScalarEvaluatorPlugin plugin("gapdb", create);

PatternGenerationEdelkamp::PatternGenerationEdelkamp(int pdb_size, int num_coll, int num_episodes,
                                                     double mutation_probability)
    : pdb_max_size(pdb_size), num_collections(num_coll) {
    initialize();
    //cout << "initial pattern collections:" << endl;
    //dump();
    vector<pair<double, int> > initial_fitness_values;
    evaluate(initial_fitness_values);
    
    double current_best_h = initial_fitness_values.back().first;
    // TODO: can do this without copying? reference as instance variable does not work (initializing problem)
    best_collection = pattern_collections[initial_fitness_values.back().second];
    
    for (int t = 0; t < num_episodes; ++t) {
        cout << endl;
        cout << "--------- episode no " << t << " ---------" << endl;
        mutate(mutation_probability);
        cout << "mutated" << endl;
        //dump();
        vector<pair<double, int> > fitness_values;
        double fitness_sum = evaluate(fitness_values);
        cout << "evaluated" << endl;
        cout << "fitness values:";
        for (size_t i = 0; i < fitness_values.size(); ++i) {
            cout << " " << fitness_values[i].first << " (index: " << fitness_values[i].second << ")";
        }
        cout << endl;
        double new_best_h = fitness_values.back().first;
        select(fitness_values, fitness_sum);
        //cout << "current pattern collections:" << endl;
        dump();
        if (new_best_h > current_best_h) {
            current_best_h = new_best_h;
            best_collection = pattern_collections[fitness_values.back().second];
        }
    }
}

PatternGenerationEdelkamp::~PatternGenerationEdelkamp() {
}

void PatternGenerationEdelkamp::dump() const {
    for (size_t i = 0; i < pattern_collections.size(); ++i) {
        cout << "pattern collection no " << i << endl;
        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            cout << "[ ";
            for (size_t k = 0; k < pattern_collections[i][j].size(); ++k) {
                cout << pattern_collections[i][j][k] << " ";
            }
            cout << "]" << endl;
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

/* TODO: Generell alle Variablen aus den
Patterns entfernen, die nicht "kausal gerechtfertigt" sind, d.h.

* Zielvariablen sind, oder
* in Vorbedingungen von Operatoren auftauchen, die bereits
kausal gerechtfertigte Variablen in den Effekten haben

Damit bekäme man kleinere Patterns und vermutlich auch mehr Duplikate,
d.h. Patterns, wo man schon auf gecachete Kosten zurückgreifen kann.
*/

void PatternGenerationEdelkamp::initialize() {
    vector<int> variables;
    variables.reserve(g_variable_domain.size());
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        variables.push_back(i);
    }
    
    for (size_t num_pcs = 0; num_pcs < num_collections; ++num_pcs) {
        random_shuffle(variables.begin(), variables.end(), g_rng);
        vector<vector<bool> > pattern_collection;
        vector<bool> pattern(g_variable_name.size(), false);
        size_t current_size = 1;
        for (size_t i = 0; i < variables.size(); ++i) {
            int var = variables[i];
            int next_var_size = g_variable_domain[var];
            if (next_var_size <= pdb_max_size) {
                if (current_size * next_var_size > pdb_max_size) {
                    if (!pattern.empty()) {
                        pattern_collection.push_back(pattern);
                        pattern.clear();
                        pattern.resize(g_variable_name.size(), false);
                        current_size = 1;
                    }
                }
                current_size *= next_var_size;
                pattern[var] = true;
            }
        }
        if (!pattern.empty())
            pattern_collection.push_back(pattern);
        pattern_collections.push_back(pattern_collection);
    }
}

/*
void PatternGenerationEdelkamp::recombine() {
    size_t vector_size = pattern_collections.size();
    for (size_t i = 1; i < vector_size; i += 2) {
        vector<vector<bool> > collection1;
        vector<vector<bool> > collection2;
        for (size_t j = 0; j < pattern_collections[i-1].size(); ++j) {
            if (j < pattern_collections[i-1].size() / 2) {
                collection1.push_back(pattern_collections[i-1][j]);
            }
            else {
                collection2.push_back(pattern_collections[i-1][j]);
            }
        }
        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            if (j < pattern_collections[i].size() / 2) {
                collection1.push_back(pattern_collections[i][j]);
            }
            else {
                collection2.push_back(pattern_collections[i][j]);
            }
        }
        pattern_collections.push_back(collection1);
        pattern_collections.push_back(collection2);
    }
}*/

void PatternGenerationEdelkamp::mutate(double probability) {
    for (size_t i = 0; i < pattern_collections.size(); ++i) {
        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            vector<bool> &pattern = pattern_collections[i][j];
            //cout << "pattern before mutate:" << endl;
            //for (size_t k = 0; k < pattern.size(); ++k) {
              //  cout << pattern_collections[i][j][k] << " ";
            //}
            //cout << endl;
            for (size_t k = 0; k < pattern.size(); ++k) {
                double random = g_rng(); // [0..1)
                if (random < probability) {
                    cout << "mutating variable no " << k << " in pattern no " << j
                    << " of pattern collection no " << i << endl;
                    pattern[k] = !pattern[k];
                }
            }
           /* cout << "pattern after mutate:" << endl;
            for (size_t k = 0; k < pattern.size(); ++k) {
                cout << pattern_collections[i][j][k] << " ";
            }
            cout << endl;*/
        }
    }
}

double PatternGenerationEdelkamp::evaluate(vector<pair<double, int> > &fitness_values) {
    double total_sum = 0;
    for (size_t i = 0; i < pattern_collections.size(); ++i) {
        cout << "evaluate pattern collection " << i << " of " << pattern_collections.size() << endl;
        double fitness = 0;
        vector<bool> variables_used(g_variable_domain.size(), false);
        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            const vector<bool> &bitvector = pattern_collections[i][j];

            // test if variables occur in more than one pattern
            // TODO: iteration through bitvector occurs here and in transformation to pattern normal form
            // any way to avoid this?
            /*bool patterns_additive = true;
            for (size_t i = 0; i < bitvector.size(); ++i) {
                if (bitvector[i]) {
                    if (variables_used[i]) {
                        cout << "patterns not additive anymore!" << endl;
                        fitness = 0.001; // HACK: for the cases in which all pattern collections are invalid,
                        // prevent gettin 0 probabilities for all entries
                        patterns_additive = false;
                        break;
                    }
                    variables_used[i] = true;
                }
            }
            if (!patterns_additive) {
                break;
            }*/
            // calculate mean h-value for actual pattern collection
            //hash_map<vector<bool>, double>::const_iterator it = pattern_to_fitness.find(bitvector);
            map<vector<bool>, double>::const_iterator it = pattern_to_fitness.find(bitvector);
            double mean_h = 0;
            if (it == pattern_to_fitness.end()) {
                vector<int> pattern;
                transform_to_pattern_normal_form(bitvector, pattern);
                //cout << "transformed into normal pattern form" << endl;
                PDBHeuristic pdb_heuristic(pattern);
                //cout << "calculated pdb" << endl;
                const vector<int> &h_values = pdb_heuristic.get_h_values();
                double sum = 0;
                int num_states = h_values.size();
                for (size_t k = 0; k < h_values.size(); ++k) {
                    if (h_values[k] == numeric_limits<int>::max()) {
                        --num_states;
                        continue;
                    }
                    sum += h_values[k];
                }
                mean_h = sum / num_states;
                pattern_to_fitness.insert(make_pair(bitvector, mean_h));
            } else {
                mean_h = (*it).second;
            }
            fitness += mean_h;
        }
        fitness_values.push_back(make_pair(fitness, i));
        total_sum += fitness;
    }
    sort(fitness_values.begin(), fitness_values.end());
    return total_sum;
}

void PatternGenerationEdelkamp::select(const vector<pair<double, int> > &fitness_values, double fitness_sum) {
    vector<double> probabilities;
    for (size_t i = 0; i < fitness_values.size(); ++i) {
        probabilities.push_back(fitness_values[i].first / fitness_sum);
        if (i > 0)
            probabilities[i] += probabilities[i-1];
    }
    cout << "summed probabilities: ";
    for (size_t i = 0; i < probabilities.size(); ++i) {
        cout << probabilities[i] << " ";
    }
    cout << endl;

    vector<vector<vector<bool> > > new_pattern_collections;
    for (size_t i = 0; i < num_collections; ++i) {
        double random = g_rng(); // [0..1)
        int j = 0;
        while (random > probabilities[j]) {
            ++j;
        }
        cout << "random = " << random << " j = " << j << " index = " << fitness_values[j].second << endl;
        new_pattern_collections.push_back(pattern_collections[fitness_values[j].second]);
    }
    pattern_collections = new_pattern_collections;
}

PDBCollectionHeuristic *PatternGenerationEdelkamp::get_pattern_collection_heuristic() {
    // return the best collection of the last pattern collections (after all episodes)
    /*vector<double> fitness_values;
    evaluate(fitness_values);
    double best_fitness = fitness_values[0];
    int best_index = 0;
    for (size_t i = 1; i < fitness_values.size(); ++i) {
        if (fitness_values[i] > best_fitness) {
            best_fitness = fitness_values[i];
            best_index = i;
        }
    }
    const vector<vector<bool> > &best_collection = pattern_collections[best_index];*/
    vector<vector<int> > pattern_collection;
    for (size_t j = 0; j < best_collection.size(); ++j) {
        vector<int> pattern;
        for (size_t i = 0; i < best_collection[j].size(); ++i) {
            if (best_collection[j][i])
                pattern.push_back(i);
        }
        if (pattern.empty())
            continue;
        pattern_collection.push_back(pattern);
    }
    return new PDBCollectionHeuristic(pattern_collection);
}

ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run) {
    int pdb_max_size = 100;
    int num_collections = 5;
    int num_episodes = 30;
    int mutation_probability = 1;
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            option_parser.add_int_option("pdb_max_size", &pdb_max_size, "max size for pdbs");
            option_parser.add_int_option("num_collections", &num_collections, "number of pattern collections to maintain");
            option_parser.add_int_option("num_episodes", &num_episodes, "number of episodes");
            option_parser.add_int_option("mutation_probability", &mutation_probability, "probability for flipping a bit");
            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        end = start;
    }

    if (pdb_max_size < 1) {
        cerr << "error: size per pdb must be at least 1" << endl;
        exit(2);
    }
    if (num_collections < 1) {
        cerr << "error: number of pattern collections must be at least 1" << endl;
        exit(2);
    }
    if (num_episodes < 1) {
        cerr << "error: number of episodes must be at least 1" << endl;
        exit(2);
    }
    if (mutation_probability < 0 || mutation_probability > 100) {
        cerr << "error: mutation probability must be in [0..100]" << endl;
        exit(2);
    }

    if (dry_run)
        return 0;
    Timer timer;
    PatternGenerationEdelkamp pge(pdb_max_size, num_collections, num_episodes, mutation_probability / 100.0);
    cout << "Pattern Generation (Edelkamp) time: " << timer << endl;
    return pge.get_pattern_collection_heuristic();
}
