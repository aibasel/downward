#include "pattern_generation_haslum.h"
#include "causal_graph.h"
#include "globals.h"
#include "operator.h"
#include "pdb_collection_heuristic.h"
#include "pdb_heuristic.h"
#include "plugin.h"
#include "rng.h"
#include "state.h"
#include "successor_generator.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

using namespace std;

static ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run);
static ScalarEvaluatorPlugin pattern_generation_haslum_plugin("ipdb", create);

PatternGenerationHaslum::PatternGenerationHaslum(int max_pdb, int max_coll, int samples)
: max_pdb_size(max_pdb), max_collection_size(max_coll), num_samples(samples) {
    // TODO: add functionality for the parameter options max_pdb_size and max_collection_size
    hill_climbing();
}

PatternGenerationHaslum::~PatternGenerationHaslum() {
}

// generates successors for the initial pattern-collection, called once before the first hillclimbing iteration
/*void PatternGenerationHaslum::initial_candidates(const PDBCollectionHeuristic &current_collection,
                                                  vector<vector<int> > &candidate_patterns) {
    assert(candidate_patterns.empty());
    for (size_t i = 0; i < current_collection.get_pattern_databases().size(); ++i) {
        const vector<int> &current_pattern = current_collection.get_pattern_databases()[i]->get_pattern();
        for (size_t j = 0; j < current_pattern.size(); ++j) {
            const vector<int> &relevant_vars = g_causal_graph->get_predecessors(current_pattern[j]);
            for (size_t k = 0; k < relevant_vars.size(); ++k) {
                vector<int> new_pattern(current_pattern);
                new_pattern.push_back(relevant_vars[k]);
                sort(new_pattern.begin(), new_pattern.end());
                candidate_patterns.push_back(new_pattern);
            }
        }
    }
}*/

// incrementally generates successors for the new best pattern (old successors always remain successors)
void PatternGenerationHaslum::generate_candidate_patterns(const vector<int> &pattern,
                                                          vector<vector<int> > &candidate_patterns) {
    for (size_t i = 0; i < pattern.size(); ++i) {
        vector<int> rel_vars = g_causal_graph->get_predecessors(pattern[i]);
        sort(rel_vars.begin(), rel_vars.end());
        vector<int> relevant_vars;
        set_difference(rel_vars.begin(), rel_vars.end(), pattern.begin(), pattern.end(), back_inserter(relevant_vars));
        for (size_t j = 0; j < relevant_vars.size(); ++j) {
            /*cout << "old pattern... ";
            for (size_t x = 0; x < pattern.size(); ++x) {
                cout << pattern[x] << " ";
            }
            cout << endl;*/
            vector<int> new_pattern(pattern);
            new_pattern.push_back(relevant_vars[j]);
            /*cout << "add " << relevant_vars[j] << " to old pattern" << endl;
            cout << "new pattern... ";
            for (size_t x = 0; x < new_pattern.size(); ++x) {
                cout << new_pattern[x] << " ";
            }
            cout << endl;*/
            sort(new_pattern.begin(), new_pattern.end());
            /*cout << "old pattern after sorting new pattern... ";
            for (size_t x = 0; x < pattern.size(); ++x) {
                cout << pattern[x] << " ";
            }
            cout << endl;*/
            candidate_patterns.push_back(new_pattern);
            /*cout << "old pattern after inserting into collection... ";
            for (size_t x = 0; x < pattern.size(); ++x) {
                cout << pattern[x] << " ";
            }
            cout << endl;
            cout << endl;*/
        }
    }
    cout << "all possible new pattern candidates" << endl;
    for (size_t i = 0; i < candidate_patterns.size(); ++i) {
        cout << "[ ";
        for (size_t j = 0; j < candidate_patterns[i].size(); ++j) {
            cout << candidate_patterns[i][j] << " ";
        }
        cout << " ]";
    }
    cout << endl;
}

// random walk for state sampling
void PatternGenerationHaslum::sample_states(vector<State> &samples) {
    // TODO update branching factor (later)
    vector<const Operator *> applicable_ops;
    g_successor_generator->generate_applicable_ops(*g_initial_state, applicable_ops);
    double b = applicable_ops.size();
    current_collection->evaluate(*g_initial_state);
    assert(!current_collection->is_dead_end());
    double d = 2.0 * current_collection->get_heuristic();
    int length = 0;
    double denominator = pow(b, d + 1) - 1;
    State current_state(*g_initial_state);
    while (true) {
        double numerator = pow(b, length + 1.0) - pow(b, length);
        double fraction = numerator / denominator;
        double random = g_rng(); // [0..1)
        if (random < fraction) {
            samples.push_back(current_state);
            break;
        }
        // restart takes place by calling this method as long as we don't have enough sample states
        
        vector<const Operator *> applicable_ops;
        g_successor_generator->generate_applicable_ops(current_state, applicable_ops);
        
        int random2 = g_rng.next(applicable_ops.size()); // [0..applicalbe_os.size())
        assert(applicable_ops[random2]->is_applicable(current_state));
        
        // get new state, 
        current_state = State(current_state, *applicable_ops[random2]);
        
        // check whether new state is a dead end (h == -1)
        current_collection->evaluate(current_state);
        if (current_collection->is_dead_end())
            break; // stop sampling. no restart, this is done by calling this method again
        ++length;
    }
}

bool PatternGenerationHaslum::counting_approximation(PDBHeuristic &pdbheuristic,
                                                     const State &sample,
                                                     PDBCollectionHeuristic *current_collection,
                                                     vector<vector<PDBHeuristic *> > &max_additive_subsets) {
    pdbheuristic.evaluate(sample);
    if (pdbheuristic.is_dead_end()) {
        cout << "dead end" << endl;
        return true;
    }
    int h_pattern = pdbheuristic.get_heuristic();
    current_collection->evaluate(sample);
    int h_collection = current_collection->get_heuristic();
    for (size_t k = 0; k < max_additive_subsets.size(); ++k) {
        int h_subset = 0;
        for (size_t l = 0; l < max_additive_subsets[k].size(); ++l) {
            max_additive_subsets[k][l]->evaluate(sample);
            assert(!max_additive_subsets[k][l]->is_dead_end());
            h_subset += max_additive_subsets[k][l]->get_heuristic();
        }
        if (h_pattern + h_subset> h_collection) {
            return true;
        }
    }
    return false;
}

void PatternGenerationHaslum::hill_climbing() {
    // initial collection: a pdb for each goal variable
    vector<vector<int> > pattern_collection;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        pattern_collection.push_back(vector<int>(1, g_goal[i].first));
    }
    current_collection = new PDBCollectionHeuristic(pattern_collection);
    
    // initial candidate patterns, computed separatedly for each pattern from the initial collection
    vector<vector<int> > candidate_patterns;
    for (size_t i = 0; i < current_collection->get_pattern_databases().size(); ++i) {
        const vector<int> &current_pattern = current_collection->get_pattern_databases()[i]->get_pattern();
        generate_candidate_patterns(current_pattern, candidate_patterns);
    }
    // remove duplicates in the final candidate list
    sort(candidate_patterns.begin(), candidate_patterns.end());
    candidate_patterns.erase(unique(candidate_patterns.begin(), candidate_patterns.end()), candidate_patterns.end());
    cout << "done calculating initial pattern collection and candidate patterns for the search" << endl;
    
    // actual hillclimbing loop
    bool improved = true;
    while (improved) {
        improved = false;
        
        // sample states until we have enough
        // TODO: resampling seems to be much slower than sampling once
        vector<State> samples;
        while (samples.size() < num_samples) {
            sample_states(samples);
        }
        
        // TODO: drop PDBHeuristic and use astar instead to compute h values for the sample states only
        // How is the advance of astar if we always have new samples? If we use pdbs and we don't rebuild
        // them in every loop, there is no problem with new samples.
        int best_pattern_count = 0;
        int best_pattern_index = 0;
        for (size_t i = 0; i < candidate_patterns.size(); ++i) {
            PDBHeuristic pdbheuristic(candidate_patterns[i]);
            // at the moment we always calculate all PDBs. But only few new successor-patterns are
            // really new. If we have enough memory, we should save all constructed pdbs.
            // (And delete them before the real search for a plan)
            vector<vector<PDBHeuristic *> > max_additive_subsets;
            current_collection->get_max_additive_subsets(candidate_patterns[i], max_additive_subsets);
            int count = 0;
            // calculate the "counting approximation" for all sample states
            // TODO: stop after m/t and use statistical confidence intervall
            for (size_t j = 0; j < samples.size(); ++j) {
                if (counting_approximation(pdbheuristic,
                                           samples[j],
                                           current_collection,
                                           max_additive_subsets))
                    ++count;
            }
            if (count > best_pattern_count) {
                best_pattern_count = count;
                best_pattern_index = i;
                improved = true;
            }
            cout << "pattern [";
            for (size_t j = 0; j < candidate_patterns[i].size(); ++j) {
                cout << candidate_patterns[i][j] << " ";
            }
            cout << "] improvement: " << count << endl;
        }
        if (improved) {
            cout << "yippieee! we found a better pattern! Its improvement is " << best_pattern_count << endl;
            cout << "pattern [";
            for (size_t i = 0; i < candidate_patterns[best_pattern_index].size(); ++i) {
                cout << candidate_patterns[best_pattern_index][i] << " ";
            }
            cout << "]" << endl;
            current_collection->add_new_pattern(candidate_patterns[best_pattern_index]);
            // Do NOT make this a const ref! one really needs to copy this pattern, as passing
            // an object as an argument which is part of a vector (the second argument) may
            // cause memory leaks, i.e. when the vector needs to be resized after an insert
            // operation the first argument is an invalid reference
            vector<int> best_pattern = candidate_patterns[best_pattern_index];
            // successors for next iteration
            generate_candidate_patterns(best_pattern, candidate_patterns);
        }
    }
}

ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run) {
    int max_pdb_size = 2000000;
    int max_collection_size = 20000000;
    int num_samples = 100;
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            option_parser.add_int_option("max_pdb_size", &max_pdb_size, "maximum size per pdb");
            option_parser.add_int_option("max_collection_size", &max_collection_size, "max collection size");
            option_parser.add_int_option("num_samples", &num_samples, "number of samples");
            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        end = start;
    }

    // TODO: required meaningful value
    if (max_pdb_size < 1) {
        cerr << "error: size per pdb must be at least 1" << endl;
        exit(2);
    }
    
    if (dry_run)
        return 0;
    
    PatternGenerationHaslum pgh(max_pdb_size, max_collection_size, num_samples);
    cout << "Haslum et al. done." << endl;
    return pgh.get_pattern_collection_heuristic();
}
