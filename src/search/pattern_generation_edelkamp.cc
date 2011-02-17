#include "pattern_generation_edelkamp.h"
#include "globals.h"
#include "pdb_collection_heuristic.h"
#include "pdb_heuristic.h"
#include "plugin.h"
#include "rng.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>

using namespace std;

static ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run);
static ScalarEvaluatorPlugin pattern_generation_edelkamp_plugin("gapdb", create);

PatternGenerationEdelkamp::PatternGenerationEdelkamp(int initial_pdb_max_size, int mcn)
    : max_collection_number(mcn) {
    int t = 0;
    initialize(initial_pdb_max_size);
    cout << "initial pattern collections" << endl;
    dump();
    vector<pair<int, int> > initial_fitness_values;
    evaluate(initial_fitness_values);
    int current_best_h = initial_fitness_values[initial_fitness_values.size()-1].first;
    while(t < 100) {
        cout << "current t = " << t << endl;
        recombine();
        cout << "recombined" << endl;
        dump();
        mutate();
        cout << "mutated" << endl;
        dump();
        vector<pair<int, int> > fitness_values;
        evaluate(fitness_values);
        cout << "evaluated" << endl;
        int new_best_h = fitness_values[fitness_values.size()-1].first;
        select(fitness_values);
        dump();
        if (new_best_h > current_best_h)
            current_best_h = new_best_h;
        else
            break;
        ++t;
    }
}

PatternGenerationEdelkamp::~PatternGenerationEdelkamp() {
}

void PatternGenerationEdelkamp::dump() const {
    cout << "current pattern collections:" << endl;
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

void PatternGenerationEdelkamp::initialize(int initial_pdb_max_size) {
    vector<int> variables;
    variables.resize(g_variable_domain.size());
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        variables[i] = i;
    }
    
    for (size_t num_pcs = 0; num_pcs < max_collection_number; ++num_pcs) {
        vector<vector<bool> > pattern_collection;
        int i = 0;
        int current_size = 1;
        vector<bool> pattern;
        pattern.resize(g_variable_name.size(), false);
        while (i < variables.size()) {
            if (current_size * g_variable_domain[variables[i]] < initial_pdb_max_size) {
                //pattern.push_back(variables[i]);
                pattern[variables[i]] = true;
                //cout << "pushing variable " << variables[i] << endl;
                current_size *= g_variable_domain[variables[i]];
                ++i;
            } else if (!pattern.empty()) {
                pattern_collection.push_back(pattern);
                /*cout << "pushing pattern into collection:" << endl;
                for (size_t j = 0; j < pattern.size(); ++j) {
                    cout << pattern[j] << " ";
                }
                cout << endl;*/
                current_size = 1;
                pattern.clear();
                pattern.resize(g_variable_name.size(), false);
            } else { //pattern.empty()
                cout << "no more variables fitted into pattern" << endl;
                ++i;
            }
        }
        pattern_collections.push_back(pattern_collection);
        random_shuffle(variables.begin(), variables.end());
        /*cout << "new variable ordering after shuffling:" << endl;
        for (size_t i = 0; i < g_variable_domain.size(); ++i) {
            cout << variables[i] << " ";
        }
        cout << endl;*/
    }
}

void PatternGenerationEdelkamp::recombine() {
    size_t vector_size = pattern_collections.size();
    for (size_t i = 1; i < vector_size; i += 2) {
        //cout << "--- new pattern combination ---" << endl;
        vector<vector<bool> > collection1;
        vector<vector<bool> > collection2;
        //cout << "pattern collection " << i-1 << endl;
        for (size_t j = 0; j < pattern_collections[i-1].size(); ++j) {
            /*cout << "pushing pattern [ ";
            for (size_t k = 0; k < pattern_collections[i-1][j].size(); ++k) {
                cout << pattern_collections[i-1][j][k] << " ";
            }*/
            if (j < pattern_collections[i-1].size() / 2) {
                //cout << "] into collection 1" << endl;
                collection1.push_back(pattern_collections[i-1][j]);
            }
            else {
                //cout << "] into collection 2" << endl;
                collection2.push_back(pattern_collections[i-1][j]);
            }
        }
        //cout << "pattern collection " << i << endl;
        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            /*cout << "pushing pattern [ ";
            for (size_t k = 0; k < pattern_collections[i][j].size(); ++k) {
                cout << pattern_collections[i][j][k] << " ";
            }*/
            if (j < pattern_collections[i].size() / 2) {
                //cout << "] into collection 1" << endl;
                collection1.push_back(pattern_collections[i][j]);
            }
            else {
                //cout << "] into collection 2" << endl;
                collection2.push_back(pattern_collections[i][j]);
            }
        }
        pattern_collections.push_back(collection1);
        pattern_collections.push_back(collection2);
        //cout << endl;
    }
}

void PatternGenerationEdelkamp::mutate() {
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
                if (random < 0.01)
                    pattern[k] = !pattern[k];
            }
           /* cout << "pattern after mutate:" << endl;
            for (size_t k = 0; k < pattern.size(); ++k) {
                cout << pattern_collections[i][j][k] << " ";
            }
            cout << endl;*/
        }
    }
}

void PatternGenerationEdelkamp::evaluate(vector<pair<int, int> > &fitness_values) const {
    for (size_t i = 0; i < pattern_collections.size(); ++i) {
        cout << "evaluate pattern collection " << i << " of " << pattern_collections.size() << endl;
        double fitness = 0;
        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            cout << "calculate mean value for pattern " << j << endl;
            vector<int> pattern;
            for (size_t k = 0; k < pattern_collections[i][j].size(); ++k) {
                if (pattern_collections[i][j][k])
                    pattern.push_back(k);
            }
            PDBHeuristic pdb_heuristic(pattern);
            const vector<int> &h_values = pdb_heuristic.get_h_values();
            int sum = 0;
            for (size_t k = 0; k < h_values.size(); ++k) {
                if (h_values[k] == numeric_limits<int>::max())
                    continue;
                sum += h_values[k];
            }
            double mean_h = sum / h_values.size();
            fitness += mean_h;
        }
        fitness_values.push_back(make_pair(fitness, i));
    }
    sort(fitness_values.begin(), fitness_values.end());
}

void PatternGenerationEdelkamp::select(const vector<pair<int, int> > &fitness_values) {
    // TODO: how to understand the paper: is the normalized fitness function a probability distribution?
    // if yes, how to chose the right number of pattern collections?
    // for the moment, we just take the x best pattern collections, where x is the number of collections
    // we should have.
    vector<vector<vector<bool> > > new_pattern_collections;
    for (size_t i = fitness_values.size() - 1; i > fitness_values.size() - max_collection_number - 1; --i) {
        new_pattern_collections.push_back(pattern_collections[fitness_values[i].second]);
    }
    pattern_collections = new_pattern_collections;
}

PDBCollectionHeuristic *PatternGenerationEdelkamp::get_pattern_collection_heuristic() const {
    // the first one is always the one with the highest fitness value (see select method)
    const vector<vector<bool> > &best_collection = pattern_collections[0];
    vector<vector<int> > pattern_collection;
    for (size_t j = 0; j < best_collection.size(); ++j) {
        vector<int> pattern;
        for (size_t k = 0; k < best_collection[j].size(); ++k) {
            if (best_collection[j][k])
                pattern.push_back(k);
        }
        if (pattern.empty())
            continue;
        pattern_collection.push_back(pattern);
    }
    return new PDBCollectionHeuristic(pattern_collection);
}

ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run) {
    int initial_pdb_max_size = 32;
    int max_collection_number = 10;
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            option_parser.add_int_option("initial_pdb_max_size", &initial_pdb_max_size, "initial max size for pdbs");
            option_parser.add_int_option("max_collection_number", &max_collection_number, "max number of pattern collections");
            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        end = start;
    }
    
    // TODO: required meaningful value
    if (initial_pdb_max_size < 10) {
        cerr << "error: size per pdb must be at least 10" << endl;
        exit(2);
    }
    if (max_collection_number < 1) {
        cerr << "error: number of pattern collections must be at least 1" << endl;
        exit(2);
    }
    
    if (dry_run)
        return 0;
    
    PatternGenerationEdelkamp pge(initial_pdb_max_size, max_collection_number);
    cout << "Edelkamp et al. done." << endl;
    return pge.get_pattern_collection_heuristic();
}
