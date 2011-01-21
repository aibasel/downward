#include "pattern_generation_haslum.h"
#include "plugin.h"
#include "pdb_collection_heuristic.h"
#include "globals.h"
#include "pdb_heuristic.h"
#include "causal_graph.h"
#include "state.h"
#include "operator.h"
#include "rng.h"
#include "successor_generator.h"

#include <vector>
#include <string>
#include <cstdlib>
#include <cassert>
#include <algorithm>
#include <math.h>

using namespace std;

static ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run);
static ScalarEvaluatorPlugin pattern_generation_haslum_plugin("haslum", create);

PatternGenerationHaslum::PatternGenerationHaslum(int max_pdb, int max_coll, int samples)
    : max_pdb_size(max_pdb), max_collection_size(max_coll), samples_number(samples) {
    // TODO: add functionality for the parameter options max_pdb_size and max_collection_size
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
                // sort pattern
                sort(new_pattern.begin(), new_pattern.end());
                successor_patterns.push_back(new_pattern);
            }
        }
    }
    cout << "all possible new pattern candidates (after first round)" << endl;
    for (size_t i = 0; i < successor_patterns.size(); ++i) {
        cout << "[ ";
        for (size_t j = 0; j < successor_patterns[i].size(); ++j) {
            cout << successor_patterns[i][j] << " ";
        }
        cout << " ]";
    }
    cout << endl;
}

// incrementally generates successors for the new best pattern (old successors always remain successors)
// TODO: if we pass pattern as reference (const not possible because of set_difference), then weird things happen,
// namely the copied "new_pattern" seems still to be able to change "pattern", which contradicts the copy constructor.
// precisely when inserting the "new_pattern" into "successor_patterns", one of the entries of "pattern" was changed
// from 1 to 1352341374 (or similar) on some logistics problem.
void PatternGenerationHaslum::generate_successors(vector<int> pattern,
                                                  vector<vector<int> > &successor_patterns) {
    //sort(pattern.begin(), pattern.end()); // TODO necessary if we sort new pattern? should work without sorting.
    for (size_t i = 0; i < pattern.size(); ++i) {
        const vector<int> &rel_vars = g_causal_graph->get_predecessors(pattern[i]);
        vector<int> relevant_vars;
        set_difference(rel_vars.begin(), rel_vars.end(), pattern.begin(), pattern.end(), back_inserter(relevant_vars));
        for (size_t j = 0; j < relevant_vars.size(); ++j) {
            cout << "old pattern... ";
            for (size_t x = 0; x < pattern.size(); ++x) {
                cout << pattern[x] << " ";
            }
            cout << endl;
            vector<int> new_pattern(pattern);
            new_pattern.push_back(relevant_vars[j]);
            cout << "add " << relevant_vars[j] << " to old pattern" << endl;
            cout << "new pattern... ";
            for (size_t x = 0; x < new_pattern.size(); ++x) {
                cout << new_pattern[x] << " ";
            }
            cout << endl;
            sort(new_pattern.begin(), new_pattern.end());
            cout << "old pattern before push_back... ";
            for (size_t x = 0; x < pattern.size(); ++x) {
                cout << pattern[x] << " ";
            }
            cout << endl;
            successor_patterns.push_back(new_pattern);
            cout << "old pattern after push_back... ";
            for (size_t x = 0; x < pattern.size(); ++x) {
                cout << pattern[x] << " ";
            }
            cout << endl;
            cout << endl;
        }
    }
    cout << "all possible new pattern candidates (after incremental call)" << endl;
    for (size_t i = 0; i < successor_patterns.size(); ++i) {
        cout << "[ ";
        for (size_t j = 0; j < successor_patterns[i].size(); ++j) {
            cout << successor_patterns[i][j] << " ";
        }
        cout << " ]";
    }
    cout << endl;
}

// random walk for state sampling
void PatternGenerationHaslum::sample_states(vector<const State *> &samples) {
    // TODO update branching factor (later)
    vector<const Operator *> applicable_ops;
    g_successor_generator->generate_applicable_ops(*g_initial_state, applicable_ops);
    double b = applicable_ops.size();
    current_collection->evaluate(*g_initial_state);
    assert(!current_collection->is_dead_end());
    double d = 2.0 * current_collection->get_heuristic();
    int length = 0;
    double denominator = pow(b, d + 1) - 1;
    State *current_state = g_initial_state;
    while (true) {
        double numerator = pow(b, length + 1.0) - pow(b, length);
        double fraction = numerator / denominator;
        double random = g_rng(); // [0..1)
        if (random < fraction) {
            samples.push_back(current_state);
            break;
        }
        // restart takes place by calling this method as long as we don't have enough sample states
        
        // TODO: whats faster: precompute applicable operators, then use a random one,
        // or get random numbers until you find an applicable operator?
        vector<const Operator *> applicable_ops;
        g_successor_generator->generate_applicable_ops(*current_state, applicable_ops);
        
        int random2 = g_rng.next(applicable_ops.size()); // [0..applicalbe_os.size())
        assert(applicable_ops[random2]->is_applicable(*current_state));
        
        // get new state, 
        current_state = new State(*current_state, *applicable_ops[random2]);
        
        // check whether new state is a dead end (h == -1)
        current_collection->evaluate(*current_state);
        if (current_collection->is_dead_end())
            break; // stop sampling. no restart, this is done by calling this method again
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
    
    // actual hillclimbing loop
    vector<vector<int> > successor_patterns;
    generate_successors(*current_collection, successor_patterns);
    bool improved = true;
    while (improved) {
        improved = false;
        // sample states until we have enough
        // TODO: this resampling (instead of only sampling once before the loop, as it was before) 
        // slows down the whole process by at least 10
        vector<const State *> samples;
        while (samples.size() < samples_number) {
            sample_states(samples);
        }
        
        // TODO: drop PDBHeuristic and use astar instead to compute h values for the sample states only
        int best_pattern_count = 0;
        int best_pattern_index = 0;
        for (size_t i = 0; i < successor_patterns.size(); ++i) {
            PDBHeuristic *pdbheuristic = new PDBHeuristic(successor_patterns[i]);
            vector<vector<PDBHeuristic *> > max_additive_subsets;
            current_collection->get_max_additive_subsets(successor_patterns[i], max_additive_subsets);
            int count = 0;
            // calculate the "counting approximation" for all sample states
            // TODO: stop after m/t and use statistical confidence intervall
            for (size_t j = 0; j < samples.size(); ++j) {
                // TODO: can h_pattern be dead_end value? only relevant vars are considered!
                pdbheuristic->evaluate(*samples[j]);
                int h_pattern = pdbheuristic->get_heuristic();
                current_collection->evaluate(*samples[j]);
                int h_collection = current_collection->get_heuristic();
                int max_h = 0;
                for (size_t k = 0; k < max_additive_subsets.size(); ++k) {
                    int h_subset = 0;
                    for (size_t l = 0; l < max_additive_subsets[k].size(); ++l) {
                        // TODO: can this value really have infitie h_value?
                        // rather not, because for the sample states, the current collection never
                        // yields an infinite value, because this is checked in the sampling step?
                        max_additive_subsets[k][l]->evaluate(*samples[j]);
                        if (max_additive_subsets[k][l]->is_dead_end()) {
                            // TODO
                            cout << "dead end" << endl;
                        }
                        h_subset += max_additive_subsets[k][l]->get_heuristic();
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
            cout << "pattern [";
            for (size_t i = 0; i < successor_patterns[best_pattern_index].size(); ++i) {
                cout << successor_patterns[best_pattern_index][i] << " ";
            }
            cout << "]" << endl;
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
    
    int max_pdb_size = -1;
    int max_collection_size = -1;
    int samples_number = -1;
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            option_parser.add_int_option("max_pdb_size", &max_pdb_size, "maximum size per pdb");
            option_parser.add_int_option("max_collection_size", &max_collection_size, "max collection size");
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
    if (max_pdb_size == -1) {
        max_pdb_size = 2000000;
    }
    // TODO: Default value
    if (max_collection_size == -1) {
        max_collection_size = 20000000;
    }
    // TODO: required meaningful value
    if (max_pdb_size < 1) {
        cerr << "error: size per pdb must be at least 1" << endl;
        exit(2);
    }
    // TODO: Default value
    if (samples_number == -1) {
        samples_number = 100;
    }
    
    PatternGenerationHaslum pgh(max_pdb_size, max_collection_size, samples_number);
    cout << "Haslum et al. done." << endl;
    return pgh.get_pattern_collection_heuristic();
}
