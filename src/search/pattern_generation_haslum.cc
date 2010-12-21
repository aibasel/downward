#include "pattern_generation_haslum.h"
#include "plugin.h"
#include "pdb_collection_heuristic.h"
#include "globals.h"
#include "pdb_heuristic.h"
#include "causal_graph.h"

#include <vector>
#include <string>
#include <cstdlib>
#include <cassert>
#include <algorithm>

using namespace std;

static ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run);
static ScalarEvaluatorPlugin pattern_generation_haslum_plugin("haslum", create);

PatternGenerationHaslum::PatternGenerationHaslum(int max_mem) : max_pdb_memory(max_mem) {
    sample_states();
    hill_climbing();
}

PatternGenerationHaslum::~PatternGenerationHaslum() {
}

void PatternGenerationHaslum::generate_successors(const PDBCollectionHeuristic &current_collection,
                                                  vector<vector<int> > &successor_patterns) {
    // TODO: super inefficient!
    for (size_t i = 0; i < current_collection.get_pattern_databases().size(); ++i) {
        vector<int> current_pattern = current_collection.get_pattern_databases()[i]->get_pattern();
        cout << "current pattern: ";
        for (size_t l = 0; l < current_pattern.size(); ++l) {
            cout << current_pattern[l] << " ";
        }
        cout << endl;
        for (size_t j = 0; j < current_pattern.size(); ++j) {
            const vector<int> &relevant_vars = g_causal_graph->get_predecessors(current_pattern[j]);
            for (size_t k = 0; k < relevant_vars.size(); ++k) {
                vector<int> new_pattern(current_pattern);
                new_pattern.push_back(relevant_vars[k]);
                successor_patterns.push_back(new_pattern);
                cout << "one successor pattern: ";
                for (size_t l = 0; l < new_pattern.size(); ++l) {
                    cout << new_pattern[l] << " ";
                }
                cout << endl;
            }
        }
    }
}

void PatternGenerationHaslum::sample_states() {
    // TODO
    samples.push_back(g_initial_state);
}

void PatternGenerationHaslum::hill_climbing() {
    // initial collection: a pdb for each goal variable
    vector<vector<int> > pattern_collection;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        pattern_collection.push_back(vector<int>(1, g_goal[i].first));
    }
    current_collection = new PDBCollectionHeuristic(pattern_collection);
    
    bool improved = true;
    while (improved) { // TODO and enough memory
        improved = false;
        // TODO: drop PDBHeuristic and use Astar instead
        vector<vector<int> > successor_patterns;
        generate_successors(*current_collection, successor_patterns);
        int best_pattern_count = 0;
        int best_pattern_index = 0;
        for (size_t i = 0; i < successor_patterns.size(); ++i) {
            PDBHeuristic *pdbheuristic = new PDBHeuristic(successor_patterns[i]);
            cout << "successor_pattern[i]" << endl;
            for (size_t x = 0; x < successor_patterns[i].size(); ++x) {
                cout << successor_patterns[i][x] << " " << endl;
            }
            vector<vector<PDBHeuristic *> > max_additive_subsets;
            current_collection->get_max_additive_subsets(successor_patterns[i], max_additive_subsets);
            int count = 0;
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
        }
        exit(2);
    }
}

ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run) {
    //OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    
    int max_pdb_memory = -1;
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            option_parser.add_int_option("max_pdb_memory", &max_pdb_memory, "maximum pdbs size");
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
    
    PatternGenerationHaslum pgh(max_pdb_memory);
    cout << "Haslum et. al done." << endl;
    return pgh.get_pattern_collection_heuristic();
}
