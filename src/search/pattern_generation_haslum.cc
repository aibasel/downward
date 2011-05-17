#include "pattern_generation_haslum.h"
#include "causal_graph.h"
#include "globals.h"
#include "operator.h"
#include "operator_cost.h"
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

PatternGenerationHaslum::PatternGenerationHaslum(const Options &opts)
    : pdb_max_size(opts.get<int>("pdb_max_size")),
      collection_max_size(opts.get<int>("collection_max_size")),
      num_samples(opts.get<int>("num_samples")),
      min_improvement(opts.get<int>("min_improvement")),
      cost_type(opts.get<int>("cost_type")) {
    Timer timer;
    hill_climbing();
    cout << "Pattern Generation (Haslum et al.) time: " << timer << endl;
}

PatternGenerationHaslum::~PatternGenerationHaslum() {
}

// incrementally generates successors for the new best pattern (old successors always remain successors)
void PatternGenerationHaslum::generate_candidate_patterns(const vector<int> &pattern,
                                                          vector<vector<int> > &candidate_patterns) {
    map<vector<int>, int>::const_iterator it = pattern_sizes.find(pattern);
    assert(it != pattern_sizes.end());
    int current_size = it->second;
    for (size_t i = 0; i < pattern.size(); ++i) {
        vector<int> rel_vars = g_causal_graph->get_predecessors(pattern[i]);
        sort(rel_vars.begin(), rel_vars.end());
        vector<int> relevant_vars;
        set_difference(rel_vars.begin(), rel_vars.end(), pattern.begin(), pattern.end(), back_inserter(relevant_vars));
        for (size_t j = 0; j < relevant_vars.size(); ++j) {
            int new_size = current_size * g_variable_domain[relevant_vars[j]];
            if (new_size < pdb_max_size) {
                vector<int> new_pattern(pattern);
                new_pattern.push_back(relevant_vars[j]);
                sort(new_pattern.begin(), new_pattern.end());
                candidate_patterns.push_back(new_pattern);
                pattern_sizes.insert(make_pair(new_pattern, new_size));
                assert(pattern_sizes.find(new_pattern) != pattern_sizes.end());
            } else {
                cout << "ignoring new pattern as candidate because it is too large" << endl;
            }
        }
    }
    /*cout << "all possible new pattern candidates" << endl;
    for (size_t i = 0; i < candidate_patterns.size(); ++i) {
        cout << "[";
        for (size_t j = 0; j < candidate_patterns[i].size(); ++j) {
            cout << " " << candidate_patterns[i][j];
        }
        cout << " ]";
    }
    cout << endl;*/
}

// random walk for state sampling
void PatternGenerationHaslum::sample_states(vector<State> &samples, double average_operator_costs) {
    // TODO: average_operator_costs should be the actual average, not
    // something rounded up to 1. If we want to round up to 1, we
    // should do it internally here.

    current_collection->evaluate(*g_initial_state);
    assert(!current_collection->is_dead_end());

    int h = current_collection->get_heuristic();
    int n;
    if (h == 0) {
        n = 10;
    } else {
        // Convert heuristic value into an approximate number of actions
        // (does nothing on unit-cost problems).
        int solution_steps_estimate = int((h / average_operator_cost) + 0.5);
        n = 4 * solution_steps_estimate;
    }
    double p = 0.5;
    // The expected walk length is np = 2 * estimated number of solution steps.
    // (We multiply by 2 because the heuristic is underestimating.)

    while (samples.size() < num_samples) {
        // calculate length of random walk accoring to a binomial distribution
        int length = 0;
        for (int i = 0; i < n; ++i) {
            double random = g_rng(); // [0..1)
            if (random < p)
                ++length;
        }

        // random walk of length length
        State current_state(*g_initial_state);
        for (int i = 1; i < length; ++i) {
            vector<const Operator *> applicable_ops;
            g_successor_generator->generate_applicable_ops(current_state, applicable_ops);
            // if there are no applicable operators --> don't walk further
            if (applicable_ops.empty()) {
                break;
            } else {
                int random = g_rng.next(applicable_ops.size()); // [0..applicalbe_os.size())
                assert(applicable_ops[random]->is_applicable(current_state));
                current_state = State(current_state, *applicable_ops[random]);
                // if current state is dead-end, then restart with initial state
                current_collection->evaluate(current_state);
                if (current_collection->is_dead_end())
                    current_state = *g_initial_state;
            }
        }
        // last state of the random walk is used as sample
        samples.push_back(current_state);
    }
}

bool PatternGenerationHaslum::counting_approximation(PDBHeuristic *pdbheuristic,
                                                     const State &sample,
                                                     PDBCollectionHeuristic *current_collection,
                                                     vector<vector<PDBHeuristic *> > &max_additive_subsets) {
    pdbheuristic->evaluate(sample);
    if (pdbheuristic->is_dead_end()) {
        //cout << "dead end" << endl;
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
    // calculate average operator costs
    int average_operator_costs = 0;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        // TODO: check if this cast is okay! doesn't compile without it
        average_operator_costs += get_adjusted_action_cost(g_operators[i], (OperatorCost) cost_type);
    }
    average_operator_costs /= g_operators.size();
    // to avoid dividing through 0
    if (average_operator_costs == 0)
        average_operator_costs = 1;
    cout << "Average operator costs: " << average_operator_costs << endl;

    int collection_size = 0;
    // initial collection: a pdb for each goal variable
    vector<vector<int> > initial_pattern_collection;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        initial_pattern_collection.push_back(vector<int>(1, g_goal[i].first));
        pattern_sizes.insert(make_pair(initial_pattern_collection[i], g_variable_domain[g_goal[i].first]));
        collection_size += g_variable_domain[g_goal[i].first];
    }
    Options opts;
    opts.set<int>("cost_type", cost_type);
    opts.set<vector<vector<int> > >("patterns", initial_pattern_collection);
    current_collection = new PDBCollectionHeuristic(opts);
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

    // actual hill climbing loop
    Timer timer;
    map<vector<int>, PDBHeuristic *> pattern_to_pdb; // cache pdbs to avoid recalculation - TODO: hash_map?
    //vector<PDBHeuristic *> pdb_cache; // cache pdbs to avoid recalculation
    int improvement = num_samples;
    while (improvement >= min_improvement) {
        cout << "current collection size is " << collection_size << endl;
        if (collection_size >= collection_max_size) {
            cout << "stopping hill climbing due to collection max size" << endl;
            break;
        }
        improvement = 0;
        vector<State> samples;
        sample_states(samples, average_operator_costs);

        // TODO: drop PDBHeuristic and use astar instead to compute h values for the sample states only
        // How is the advance of astar if we always have new samples? If we use pdbs and we don't rebuild
        // them in every loop, there is no problem with new samples.
        int best_pattern_count = 0;
        int best_pattern_index = 0;
        PDBHeuristic *pdbheuristic;
        for (size_t i = 0; i < candidate_patterns.size(); ++i) {
            map<vector<int>, PDBHeuristic *>::const_iterator it = pattern_to_pdb.find(candidate_patterns[i]);
            if (it == pattern_to_pdb.end()) {
                Options opts;
                opts.set<int>("cost_type", cost_type);
                opts.set<vector<int> >("pattern", candidate_patterns[i]);
                pdbheuristic = new PDBHeuristic(opts);
                pattern_to_pdb.insert(make_pair(candidate_patterns[i], pdbheuristic));
            } else {
                pdbheuristic = it->second;
            }
            /*if (i < pdb_cache.size()) {
                pdbheuristic = pdb_cache[i];
            } else {
                pdbheuristic = new PDBHeuristic(candidate_patterns[i], false);
                pdb_cache.push_back(pdbheuristic);
            }*/
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
                improvement = count;
            }
            if (count > 0) {
                cout << "pattern [";
                for (size_t j = 0; j < candidate_patterns[i].size(); ++j) {
                    cout << " " << candidate_patterns[i][j];
                }
                cout << " ] improvement: " << count << endl;
            }
        }
        if (improvement >= min_improvement) {
            cout << "found a better pattern with improvement " << best_pattern_count << endl;
            cout << "pattern [";
            for (size_t i = 0; i < candidate_patterns[best_pattern_index].size(); ++i) {
                cout << " " << candidate_patterns[best_pattern_index][i];
            }
            cout << " ]" << endl;
            current_collection->add_new_pattern(candidate_patterns[best_pattern_index]);
            map<vector<int>, int>::const_iterator it = pattern_sizes.find(candidate_patterns[best_pattern_index]);
            assert(it != pattern_sizes.end());
            collection_size += it->second;
            // Do NOT make this a const ref! one really needs to copy this pattern, as passing
            // an object as an argument which is part of a vector (the second argument) may
            // cause memory leaks, i.e. when the vector needs to be resized after an insert
            // operation the first argument is an invalid reference
            vector<int> best_pattern = candidate_patterns[best_pattern_index];
            // successors for next iteration
            generate_candidate_patterns(best_pattern, candidate_patterns);
        }
        cout << "Actual time (hill climbing loop): " << timer << endl;
    }
    // delete all created pattern databases in the map
    map<vector<int>, PDBHeuristic *>::iterator it = pattern_to_pdb.begin();
    while (it != pattern_to_pdb.end()) {
        delete it->second;
        ++it;
    }
    /*for (size_t i = 0; i < pdb_cache.size(); ++i) {
        delete pdb_cache[i];
    }*/
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<int>("pdb_max_size", 2000000,
                           "max number of states per pdb");
    parser.add_option<int>("collection_max_size", 20000000,
                           "max number of states for collection");
    parser.add_option<int>("num_samples", 1000, "number of samples");
    parser.add_option<int>("min_improvement", 10,
                           "minimum improvement while hill climbing");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (opts.get<int>("pdb_max_size") < 1)
        parser.error("size per pdb must be at least 1");
    if (opts.get<int>("collection_max_size") < 1)
        parser.error("total pdb collection size must be at least 1");
    if (opts.get<int>("min_improvement") < 1)
        parser.error("minimum improvement must be at least 1");
    if (opts.get<int>("min_improvement") > opts.get<int>("num_samples"))
        parser.error("minimum improvement mustn't be higher than number of samples");

    if (parser.dry_run())
        return 0;

    PatternGenerationHaslum pgh(opts);
    return pgh.get_pattern_collection_heuristic();
}

static Plugin<ScalarEvaluator> _plugin("ipdb", _parse);
