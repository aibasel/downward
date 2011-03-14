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
#include "timer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

using namespace std;

static ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run);
static ScalarEvaluatorPlugin plugin("ipdb", create);

PatternGenerationHaslum::PatternGenerationHaslum(int max_pdb, int max_coll, int samples)
: max_pdb_size(max_pdb), max_collection_size(max_coll), num_samples(samples) {
    // TODO: add functionality for the parameter options max_pdb_size and max_collection_size

    // calculate average operator costs
    average_operator_costs = 0;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        average_operator_costs += g_operators[i].get_cost();
    }
    average_operator_costs /= g_operators.size();
    // to avoid dividing through 0
    if (average_operator_costs == 0)
        average_operator_costs = 1;
    cout << "Average operator costs of this problem: " << average_operator_costs << endl;
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
    // calculate length of random walk accoring to a binomial distribution
    current_collection->evaluate(*g_initial_state);
    assert(!current_collection->is_dead_end());
    // TODO: handle problems with high action costs. Average costs
    // of operators instead of initial states h-value
    double h = current_collection->get_heuristic();
    h /= average_operator_costs;
    // TODO: hack! (prevent division by 0)
    if (h == 0)
        h = 10;
    double mean = 2 * h + 1;
    double n = 4 * h;
    double p = mean / n;
    int length = 0;
    for (int i = 0; i < n; ++i) {
        double random = g_rng(); // [0..1)
        if (random < p)
            ++length;
    }
    
    // sample length many states
    State current_state(*g_initial_state);
    samples.push_back(current_state);
    for (int i = 1; i < length; ++i) {
        vector<const Operator *> applicable_ops;
        g_successor_generator->generate_applicable_ops(current_state, applicable_ops);
        // if there are no applicable operators --> dead end
        if (applicable_ops.size() == 0) {
            current_state = *g_initial_state;
        } else {
            int random = g_rng.next(applicable_ops.size()); // [0..applicalbe_os.size())
            assert(applicable_ops[random]->is_applicable(current_state));
            current_state = State(current_state, *applicable_ops[random]);
            // if current state is dead-end, then restart with initial state
            current_collection->evaluate(current_state);
            if (current_collection->is_dead_end())
                current_state = *g_initial_state;
        }
        samples.push_back(current_state);
    }
    /*
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
    }*/
}

bool PatternGenerationHaslum::counting_approximation(PDBHeuristic *pdbheuristic,
                                                     const State &sample,
                                                     PDBCollectionHeuristic *current_collection,
                                                     vector<vector<PDBHeuristic *> > &max_additive_subsets) {
    pdbheuristic->evaluate(sample);
    if (pdbheuristic->is_dead_end()) {
        cout << "dead end" << endl;
        return true;
    }
    int h_pattern = pdbheuristic->get_heuristic();
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
    vector<vector<int> > initial_pattern_collection;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        initial_pattern_collection.push_back(vector<int>(1, g_goal[i].first));
    }
    current_collection = new PDBCollectionHeuristic(initial_pattern_collection);
    current_collection->evaluate(*g_initial_state);
    if (current_collection->is_dead_end())
        return;
    
    // initial candidate patterns, computed separately for each pattern from the initial collection
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
        vector<State> samples;
        sample_states(samples);
        
        // TODO: drop PDBHeuristic and use astar instead to compute h values for the sample states only
        // How is the advance of astar if we always have new samples? If we use pdbs and we don't rebuild
        // them in every loop, there is no problem with new samples.
        int best_pattern_count = 0;
        int best_pattern_index = 0;
        PDBHeuristic *pdbheuristic;
        for (size_t i = 0; i < candidate_patterns.size(); ++i) {
            map<vector<int>, PDBHeuristic *>::const_iterator it = pattern_to_pdb.find(candidate_patterns[i]);
            if (it == pattern_to_pdb.end()) {
                pdbheuristic = new PDBHeuristic(candidate_patterns[i]);
                pattern_to_pdb.insert(make_pair(candidate_patterns[i], pdbheuristic));
            } else {
                pdbheuristic = (*it).second;
            }
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
    // TODO Does this swap release the memory?
    map<vector<int>, PDBHeuristic *>().swap(pattern_to_pdb);
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

    if (max_pdb_size < 1) {
        cerr << "error: size per pdb must be at least 1" << endl;
        exit(2);
    }
    
    if (dry_run)
        return 0;
    Timer timer;
    PatternGenerationHaslum pgh(max_pdb_size, max_collection_size, num_samples);
    cout << "Pattern Generation (Haslum et al.) time: " << timer << endl;
    return pgh.get_pattern_collection_heuristic();
}
