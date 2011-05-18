#include "pattern_generation_haslum.h"
#include "causal_graph.h"
#include "globals.h"
#include "operator.h"
#include "option_parser.h"
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
      cost_type(OperatorCost(opts.get<int>("cost_type"))) {
    Timer timer;
    initialize();
    cout << "Pattern Generation (Haslum et al.) time: " << timer << endl;
}

PatternGenerationHaslum::~PatternGenerationHaslum() {
}

void PatternGenerationHaslum::generate_candidate_patterns(const vector<int> &pattern,
                                                          vector<vector<int> > &candidate_patterns) {
    int current_size = current_heuristic->get_size();
    for (size_t i = 0; i < pattern.size(); ++i) {
        // causally relevant variables for current varibale from pattern
        vector<int> rel_vars = g_causal_graph->get_predecessors(pattern[i]);
        sort(rel_vars.begin(), rel_vars.end());
        vector<int> relevant_vars;
        // make sure we only use relevant variables which are not already included in pattern
        set_difference(rel_vars.begin(), rel_vars.end(), pattern.begin(), pattern.end(), back_inserter(relevant_vars));
        for (size_t j = 0; j < relevant_vars.size(); ++j) {
            // test against overflow and pdb_max_size
            double result = pdb_max_size / g_variable_domain[relevant_vars[j]];
            if (current_size <= result) { // current_size * g_variable_domain[relevant_vars[j]] <= pdb_max_size
                vector<int> new_pattern(pattern);
                new_pattern.push_back(relevant_vars[j]);
                sort(new_pattern.begin(), new_pattern.end());
                candidate_patterns.push_back(new_pattern);
            } else {
                cout << "ignoring new pattern as candidate because it is too large" << endl;
            }
        }
    }
}

void PatternGenerationHaslum::sample_states(vector<State> &samples, double average_operator_costs) {
    current_heuristic->evaluate(*g_initial_state);
    assert(!current_heuristic->is_dead_end());

    int h = current_heuristic->get_heuristic();
    int n;
    if (h == 0) {
        n = 10;
    } else {
        // Convert heuristic value into an approximate number of actions
        // (does nothing on unit-cost problems).
        // average_operator_costs cannot equal 0, as in this case, all operators
        // must have costs of 0 and in this case the if-clause triggers.
        int solution_steps_estimate = int((h / average_operator_costs) + 0.5);
        n = 4 * solution_steps_estimate;
    }
    double p = 0.5;
    // The expected walk length is np = 2 * estimated number of solution steps.
    // (We multiply by 2 because the heuristic is underestimating.)

    samples.reserve(num_samples);
    for (int i = 0; i < num_samples; ++i) {
        // calculate length of random walk accoring to a binomial distribution
        int length = 0;
        for (int j = 0; j < n; ++j) {
            double random = g_rng(); // [0..1)
            if (random < p)
                ++length;
        }

        // random walk of length length
        State current_state(*g_initial_state);
        for (int j = 0; j < length; ++j) {
            vector<const Operator *> applicable_ops;
            g_successor_generator->generate_applicable_ops(current_state, applicable_ops);
            // if there are no applicable operators --> do not walk further
            if (applicable_ops.empty()) {
                break;
            } else {
                int random = g_rng.next(applicable_ops.size()); // [0..applicalbe_os.size())
                assert(applicable_ops[random]->is_applicable(current_state));
                current_state = State(current_state, *applicable_ops[random]);
                // if current state is dead-end, then restart with initial state
                current_heuristic->evaluate(current_state);
                if (current_heuristic->is_dead_end())
                    current_state = *g_initial_state;
            }
        }
        // last state of the random walk is used as sample
        samples.push_back(current_state);
    }
}

bool PatternGenerationHaslum::is_heuristic_improved(PDBHeuristic *pdb_heuristic,
                                                     const State &sample) {
    pdb_heuristic->evaluate(sample);
    if (pdb_heuristic->is_dead_end()) {
        return true;
    }
    int h_pattern = pdb_heuristic->get_heuristic(); // h-value of the new pattern
    vector<vector<PDBHeuristic *> > max_additive_subsets;
    current_heuristic->get_max_additive_subsets(pdb_heuristic->get_pattern(), max_additive_subsets);
    current_heuristic->evaluate(sample);
    int h_collection = current_heuristic->get_heuristic(); // h-value of the current collection heuristic
    for (size_t k = 0; k < max_additive_subsets.size(); ++k) { // for each max additive subset...
        int h_subset = 0;
        for (size_t l = 0; l < max_additive_subsets[k].size(); ++l) { // ...calculate its h-value
            max_additive_subsets[k][l]->evaluate(sample);
            assert(!max_additive_subsets[k][l]->is_dead_end());
            h_subset += max_additive_subsets[k][l]->get_heuristic();
        }
        if (h_pattern + h_subset> h_collection) { 
            // return true if one max additive subest is found for which the condition is met
            return true;
        }
    }
    return false;
}

void PatternGenerationHaslum::hill_climbing(int average_operator_costs,
                                            vector<vector<int> > &candidate_patterns) {
    Timer timer;
    map<vector<int>, PDBHeuristic *> pattern_to_pdb; // Cache PDBs to avoid recalculation
    //vector<PDBHeuristic *> pdb_cache; // cache pdbs to avoid recalculation
    while (true) {
        cout << "current collection size is " << current_heuristic->get_size() << endl;
        vector<State> samples;
        sample_states(samples, average_operator_costs);

        // TODO: drop PDBHeuristic and use astar instead to compute h values for the sample states only
        // How is the advance of astar if we always have new samples? If we use pdbs and we do not rebuild
        // them in every loop, there is no problem with new samples.
        int improvement = 0; // best improvement (= hightest count) for a pattern so far
        int best_pattern_index = 0;
        PDBHeuristic *pdb_heuristic;
        // Iterate over all candidates and search for the best improving pattern
        for (size_t i = 0; i < candidate_patterns.size(); ++i) {
            map<vector<int>, PDBHeuristic *>::iterator it = pattern_to_pdb.find(candidate_patterns[i]);
            if (it == pattern_to_pdb.end()) {
                Options opts;
                opts.set<int>("cost_type", cost_type);
                opts.set<vector<int> >("pattern", candidate_patterns[i]);
                pdb_heuristic = new PDBHeuristic(opts, false);
                pattern_to_pdb.insert(make_pair(candidate_patterns[i], pdb_heuristic));
            } else if (it->second == 0) { // candidate pattern is too large
                continue;
            } else {
                pdb_heuristic = it->second;
            }
            // If a candidate's size added to the current collection's size exceed the maximum
            // collection size, then delete the PDB and let the map's entry point to a null reference
            if (current_heuristic->get_size() + pdb_heuristic->get_size() > collection_max_size) {
                delete it->second;
                it->second = 0;
                //cout << "Candidate pattern together with the collection is above the"
                    //"size limit, ignoring from now on" << endl;
                continue;
            }
            /*if (i < pdb_cache.size()) {
                pdb_heuristic = pdb_cache[i];
            } if (i >= pdb_cache.size()) {
                Options opts;
                opts.set<int>("cost_type", cost_type);
                opts.set<vector<int> >("pattern", candidate_patterns[i]);
                pdb_heuristic = new PDBHeuristic(opts, false);
                pdb_cache.push_back(pdb_heuristic);
            } else if (pdb_cache[i] == 0) { // candidate pattern is too large
                continue;
            } else {
                pdb_heuristic = pdb_cache[i];
            }
            // If a candidate's size added to the current collection's size exceed the maximum
            // collection size, then delete the PDB and let the vector's entry point to a null reference
            if (current_heuristic->get_size() + pdb_heuristic->get_size() > collection_max_size) {
                delete pdb_cache[i];
                pdb_cache[i] = 0;
                //cout << "Candidate pattern together with the collection is above the"
                    //"size limit, ignoring from now on" << endl;
                continue;
            }*/

            // Calculate the "counting approximation" for all sample states: count the number of
            // samples for which the current pattern collection heuristic would be improved
            // if the new pattern was included into it.
            // TODO: stop after m/t and use statistical confidence intervall
            int count = 0;
            for (size_t j = 0; j < samples.size(); ++j) {
                if (is_heuristic_improved(pdb_heuristic, samples[j]))
                    ++count;
            }
            if (count > improvement) {
                improvement = count;
                best_pattern_index = i;
            }
            if (count > 0) {
                cout << "pattern [";
                for (size_t j = 0; j < candidate_patterns[i].size(); ++j) {
                    cout << " " << candidate_patterns[i][j];
                }
                cout << " ] improvement: " << count << endl;
            }
        }
        if (improvement < min_improvement)
            break;
        cout << "found a better pattern with improvement " << improvement << endl;
        cout << "pattern [";
        for (size_t i = 0; i < candidate_patterns[best_pattern_index].size(); ++i) {
            cout << " " << candidate_patterns[best_pattern_index][i];
        }
        cout << " ]" << endl;
        current_heuristic->add_pattern(candidate_patterns[best_pattern_index]);

        // Do NOT make this a const ref! one really needs to copy this pattern, as passing
        // an object as an argument which is part of a vector (the second argument) may
        // cause memory leaks, i.e. when the vector needs to be resized after an insert
        // operation the first argument is an invalid reference
        vector<int> best_pattern = candidate_patterns[best_pattern_index];
        // successors for next iteration
        // TODO: check if there could be duplicates in candidate_patterns after this call
        generate_candidate_patterns(best_pattern, candidate_patterns);
        cout << "Actual time (hill climbing iteration): " << timer << endl;
    }

    // delete all created PDB-pointer in the map
    map<vector<int>, PDBHeuristic *>::iterator it = pattern_to_pdb.begin();
    while (it != pattern_to_pdb.end()) {
        delete it->second;
        ++it;
    }
    /*// delete all created PDB-pointer in the vector
    for (size_t i = 0; i < pdb_cache.size(); ++i) {
        delete pdb_cache[i];
    }*/
}

void PatternGenerationHaslum::initialize() {
    // calculate average operator costs
    double average_operator_costs = 0;
    for (size_t i = 0; i < g_operators.size(); ++i) {
        average_operator_costs += get_adjusted_action_cost(g_operators[i], cost_type);
    }
    average_operator_costs /= g_operators.size();
    cout << "Average operator costs: " << average_operator_costs << endl;

    // initial collection: a pdb for each goal variable
    vector<vector<int> > initial_pattern_collection;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        initial_pattern_collection.push_back(vector<int>(1, g_goal[i].first));
    }
    Options opts;
    opts.set<int>("cost_type", cost_type);
    opts.set<vector<vector<int> > >("patterns", initial_pattern_collection);
    current_heuristic = new PDBCollectionHeuristic(opts);
    current_heuristic->evaluate(*g_initial_state);
    if (current_heuristic->is_dead_end())
        return;

    // initial candidate patterns, computed separately for each pattern from the initial collection
    vector<vector<int> > candidate_patterns;
    for (size_t i = 0; i < current_heuristic->get_pattern_databases().size(); ++i) {
        const vector<int> &current_pattern = current_heuristic->get_pattern_databases()[i]->get_pattern();
        generate_candidate_patterns(current_pattern, candidate_patterns);
    }
    // remove duplicates in the candidate list
    sort(candidate_patterns.begin(), candidate_patterns.end());
    candidate_patterns.erase(unique(candidate_patterns.begin(), candidate_patterns.end()), candidate_patterns.end());
    cout << "done calculating initial pattern collection and candidate patterns for the search" << endl;

    hill_climbing(average_operator_costs, candidate_patterns);
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
        parser.error("minimum improvement must not be higher than number of samples");

    if (parser.dry_run())
        return 0;

    PatternGenerationHaslum pgh(opts);
    return pgh.get_pattern_collection_heuristic();
}

static Plugin<ScalarEvaluator> _plugin("ipdb", _parse);
