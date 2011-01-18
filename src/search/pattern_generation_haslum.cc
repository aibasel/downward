#include "pattern_generation_haslum.h"
#include "plugin.h"
#include "pdb_collection_heuristic.h"
#include "globals.h"
#include "pdb_heuristic.h"
#include "causal_graph.h"
#include "state.h"
#include "operator.h"

#include <vector>
#include <string>
#include <cstdlib>
#include <cassert>
#include <algorithm>
#include <math.h>

using namespace std;

static ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run);
static ScalarEvaluatorPlugin pattern_generation_haslum_plugin("haslum", create);

PatternGenerationHaslum::PatternGenerationHaslum(int max_mem, int samples)
    : max_pdb_memory(max_mem), samples_number(samples) {
    // TODO: add functionality for max_memory and possibly for max number of pdbs, max variables/pdb etc.
    hill_climbing();
}

PatternGenerationHaslum::~PatternGenerationHaslum() {
}

// generates successors for the initial pattern-collection, called once before the first hillclimbing iteration
void PatternGenerationHaslum::generate_successors(const PDBCollectionHeuristic &current_collection,
                                                  vector<vector<int> > &successor_patterns) {
    // TODO: more efficiency?
    for (size_t i = 0; i < current_collection.get_pattern_databases().size(); ++i) {
        const vector<int> &current_pattern = current_collection.get_pattern_databases()[i]->get_pattern();
        for (size_t j = 0; j < current_pattern.size(); ++j) {
            const vector<int> &relevant_vars = g_causal_graph->get_predecessors(current_pattern[j]);
            for (size_t k = 0; k < relevant_vars.size(); ++k) {
                vector<int> new_pattern(current_pattern);
                new_pattern.push_back(relevant_vars[k]);
                successor_patterns.push_back(new_pattern);
            }
        }
    }
}

// incrementally generates successors for the new best pattern (old successors always remain successors)
void PatternGenerationHaslum::generate_successors(vector<int> &pattern,
                                                  vector<vector<int> > &successor_patterns) {
    sort(pattern.begin(), pattern.end());
    for (size_t i = 0; i < pattern.size(); ++i) {
        const vector<int> &rel_vars = g_causal_graph->get_predecessors(pattern[i]);
        const vector<int> &relevant_vars;
        set_difference(rel_vars.begin(), rel_vars.end(), pattern.begin(), pattern.end(), back_inserter(relevant_vars));
        for (size_t j = 0; j < relevant_vars.size(); ++j) {
            vector<int> new_pattern(pattern);
            new_pattern.push_back(relevant_vars[j]);
            successor_patterns.push_back(new_pattern);
        }
    }
}

// random walk for state sampling
void PatternGenerationHaslum::sample_states() {
    srand(time(NULL));
    double b = 2.0; // TODO: correct branching factor?
    double d = 2.0 * current_collection->compute_heuristic(*g_initial_state);
    int length = 0;
    float denominator = pow(b, d + 1) - 1;
    State *current_state = g_initial_state;
    while (true) {
        float numerator = pow(b, length + 1.0) - pow(b, length);
        double fraction = numerator / denominator;
        float random = (rand() % 101) / 100.0; // 0.00, 0.01, ... 1.0
        if (random <= fraction) {
            samples.push_back(current_state);
            break;
        }
        
        // TODO: whats faster: precompute applicable operators, then use a random one,
        // or get random numbers until you find an applicable operator?
        vector<const Operator *> applicable_operators;
        for (size_t i = 0; i < g_operators.size(); ++i) {
            if (g_operators[i].is_applicable(*current_state))
                applicable_operators.push_back(g_operators[i]);
        }
        int random2 = rand() % applicable_operators.size();
        assert(applicable_operators[random].is_applicable(*current_state));
        
        // get new state, 
        current_state = new State(*current_state, applicable_operators[random2]);
        
        // check whether new state is a dead end (h == -1)
        // restart takes place by calling sample_states() as long as we don't have enough sample states
        if (current_collection->compute_heuristic(*current_state) == -1)
            break;
        ++length;
    }
}

void PatternGenerationHaslum::hill_climbing() {
    // initial collection: a pdb for each goal variable
    vector<vector<int> > pattern_collection;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        pattern_collection.push_back(vector<int>(1, g_goal[i].first));
    }
    current_collection = new PDBCollectionHeuristic(pattern_collection);
    
    // sample states until we have enough
    while (samples.size() < samples_number) {
        sample_states();
    }
    
    // actual hillclimbing loop
    vector<vector<int> > successor_patterns;
    generate_successors(*current_collection, successor_patterns);
    bool improved = true;
    while (improved) {
        improved = false;
        // TODO: drop PDBHeuristic and use astar instead to compute h values for the sample states only
        int best_pattern_count = 0;
        int best_pattern_index = 0;
        for (size_t i = 0; i < successor_patterns.size(); ++i) {
            PDBHeuristic *pdbheuristic = new PDBHeuristic(successor_patterns[i]);
            vector<vector<PDBHeuristic *> > max_additive_subsets;
            current_collection->get_max_additive_subsets(successor_patterns[i], max_additive_subsets);
            int count = 0;
            // calculate the "counting approximation" for all sample states
            for (size_t j = 0; j < samples.size(); ++j) {
                int h_pattern = pdbheuristic->compute_heuristic(*samples[j]);
                int h_collection = current_collection->compute_heuristic(*samples[j]);
                int max_h = 0;
                for (size_t k = 0; k < max_additive_subsets.size(); ++k) {
                    int h_subset = 0;
                    for (size_t l = 0; l < max_additive_subsets[k].size(); ++l) {
                        h_subset += max_additive_subsets[k][l]->compute_heuristic(*samples[j]);
                    }
                    max_h = max(max_h, h_subset);
                }
                if (h_pattern > h_collection - max_h) {
                    ++count;
                }
            }
            if (count > best_pattern_count) {
                best_pattern_count = count;
                best_pattern_index = i;
                improved = true;
            }
        }
        if (improved) {
            cout << "yippieee! we found a better pattern!" << endl;
            current_collection->add_new_pattern(successor_patterns[best_pattern_index]);
            // successors for next iteration
            generate_successors(successor_patterns[best_pattern_index], successor_patterns);
        }
    }
}

ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run) {
    //OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    
    int max_pdb_memory = -1;
    int samples_number = -1;
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            option_parser.add_int_option("max_pdb_memory", &max_pdb_memory, "maximum pdbs size");
            option_parser.add_int_option("samples_number", &samples_number, "number of samples");
            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        end = start;
    }
    // TODO: Default value
    if (max_pdb_memory == -1) {
        max_pdb_memory = 1000000;
    }
    // TODO: required meaningful value
    if (max_pdb_memory < 1) {
        cerr << "error: abstraction size must be at least 1" << endl;
        exit(2);
    }
    // TODO: Default value
    if (samples_number == -1) {
        samples_number = 100;
    }
    
    PatternGenerationHaslum pgh(max_pdb_memory, samples_number);
    cout << "Haslum et al. done." << endl;
    return pgh.get_pattern_collection_heuristic();
}
