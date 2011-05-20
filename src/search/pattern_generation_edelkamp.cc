#include "pattern_generation_edelkamp.h"
#include "causal_graph.h"
#include "globals.h"
#include "operator.h" // for the use of g_operators
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

PatternGenerationEdelkamp::PatternGenerationEdelkamp(const Options &opts)
    : Heuristic(opts), pdb_max_size(opts.get<int>("pdb_max_size")),
    num_collections(opts.get<int>("num_collections")),
    num_episodes(opts.get<int>("num_episodes")),
    mutation_probability(opts.get<int>("mutation_probability") / 100.0),
    disjoint_patterns(opts.get<bool>("disjoint")),
    cost_type(opts.get<int>("cost_type")) {
    Timer timer;
    genetic_algorithm();
    cout << "Pattern Generation (Edelkamp) time: " << timer << endl;
}

PatternGenerationEdelkamp::~PatternGenerationEdelkamp() {
    // TODO: check correctness - apparently this destructor is never called anyways
    /*cout << "destructor called" << endl;
    for (size_t i = 0; i < final_pattern_collection.size(); ++i) {
        delete final_pattern_collection[i];
    }*/
}

void PatternGenerationEdelkamp::select(const vector<pair<double, int> > &fitness_values, double fitness_sum) {
    vector<double> probabilities;
    for (size_t i = 0; i < fitness_values.size(); ++i) {
        probabilities.push_back(fitness_values[i].first / fitness_sum);
        if (i > 0)
            probabilities[i] += probabilities[i-1];
    }
    /*cout << "summed probabilities: ";
    for (size_t i = 0; i < probabilities.size(); ++i) {
        cout << probabilities[i] << " ";
    }
    cout << endl;*/
    
    vector<vector<vector<bool> > > new_pattern_collections;
    for (size_t i = 0; i < num_collections; ++i) {
        double random = g_rng(); // [0..1)
        int j = 0;
        while (random > probabilities[j]) {
            ++j;
        }
        //cout << "random = " << random << " j = " << j << " index = " << fitness_values[j].second << endl;
        new_pattern_collections.push_back(pattern_collections[fitness_values[j].second]);
    }
    pattern_collections = new_pattern_collections;
}

void PatternGenerationEdelkamp::mutate() {
    // TODO: Should we check the max pdb size here? After mutation new variables can occur in a pattern and
    // exceed the max_pdb_size!
    for (size_t i = 0; i < pattern_collections.size(); ++i) {
        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            vector<bool> &pattern = pattern_collections[i][j];
            for (size_t k = 0; k < pattern.size(); ++k) {
                double random = g_rng(); // [0..1)
                if (random < mutation_probability) {
                    /*cout << "mutating variable no " << k << " in pattern no " << j
                    << " of pattern collection no " << i << endl;*/
                    pattern[k] = !pattern[k];
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

double PatternGenerationEdelkamp::evaluate(vector<pair<double, int> > &fitness_values) {
    double total_sum = 0;
    for (size_t i = 0; i < pattern_collections.size(); ++i) {
        cout << "evaluate pattern collection " << i << " of " << (pattern_collections.size() - 1) << endl;
        double fitness = 0;
        vector<bool> variables_used(g_variable_domain.size(), false);
        vector<int> &op_costs = operator_costs[i]; // operator costs for actual pattern collection
        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            const vector<bool> &bitvector = pattern_collections[i][j];
            // test if the pattern respects the memory limit
            int mem = 1;
            for (size_t k = 0; k < bitvector.size(); ++k) {
                if (bitvector[k]) {
                    // test against overflow and pdb_max_size
                    if (mem <= pdb_max_size / g_variable_domain[k]) {
                        // mem * g_variable_domain[k] <= pdb_max_size
                        mem *= g_variable_domain[k];
                    } else {
                        cout << "pattern " << j << " exceeds the memory limit!" << endl;
                        fitness = 0.001;
                        break;
                    }
                }
            }
            if (fitness == 0.001)
                break;
            // TODO: maybe combine those two loops over bitvector?
            // test if variables occur in more than one pattern
            // TODO: iteration through bitvector occurs here and in transformation to pattern normal form
            // any way to avoid this?
            if (disjoint_patterns) {
                bool patterns_disjoint = true;
                for (size_t k = 0; k < bitvector.size(); ++k) {
                    if (bitvector[k]) {
                        if (variables_used[k]) {
                            cout << "patterns are not disjoint anymore!" << endl;
                            fitness = 0.001; // HACK: for the cases in which all pattern collections are invalid,
                                        // prevent getting 0 probabilities for all entries
                            patterns_disjoint = false;
                            break;
                        }
                        variables_used[k] = true;
                    }
                }
                if (!patterns_disjoint) {
                    break;
                }
            }

            // try to remove irrelevant variables
            vector<bool> modified_bitvector;
            modified_bitvector.resize(bitvector.size());
            vector<int> vars_to_check;
            for (size_t k = 0; k < g_goal.size(); ++k) {
                if (bitvector[g_goal[k].first] == 1) {
                    vars_to_check.push_back(g_goal[k].first);
                    modified_bitvector[g_goal[k].first] = 1; // because it's causal relevant
                }
            }

            while(!vars_to_check.empty()) {
                int var = vars_to_check.back();
                vars_to_check.pop_back();
                vector<int> rel = g_causal_graph->get_predecessors(var);
                for (size_t l = 0; l < rel.size(); ++l) {
                    if (rel[l] != var) {
                        if (bitvector[rel[l]] == 1) {
                            if (modified_bitvector[rel[l]] == 0) {
                                modified_bitvector[rel[l]] = 1; // because it's causal relevant
                                vars_to_check.push_back(rel[l]);
                            }
                        }
                    }
                }
            }

            /*cout << "bitvector:" << endl;
            for (size_t k = 0; k < bitvector.size(); ++k) {
                cout << bitvector[k] << " ";
            }
            cout << endl;
            cout << "modified bitvector:" << endl;
            for (size_t k = 0; k < modified_bitvector.size(); ++k) {
                cout << modified_bitvector[k] << " ";
            }
            cout << endl;*/

            // calculate mean h-value for actual pattern collection
            //hash_map<vector<bool>, double>::const_iterator it = pattern_to_fitness.find(bitvector);
            map<vector<bool>, double>::const_iterator it = pattern_to_fitness.find(modified_bitvector);
            double mean_h = 0;
            if (it == pattern_to_fitness.end()) {
                vector<int> pattern;
                transform_to_pattern_normal_form(modified_bitvector, pattern);
                //cout << "transformed into normal pattern form" << endl;
                /*cout << "[";
                if (pattern.size() == 0)
                    cout << "empty pattern" << endl;
                else
                    cout << "try to build the pdb with the following pattern" << endl;
                for (int i = 0; i < pattern.size(); ++i) {
                    cout << pattern[i] << ", ";
                }
                cout << "]" << endl;*/

                Options opts;
                opts.set<int>("cost_type", cost_type);
                opts.set<vector<int> >("pattern", pattern);
                PDBHeuristic pdb_heuristic(opts, false, op_costs);

                // get used operators and set their cost for further iterations to 0 (action cost partitioning)
                const vector<bool> &used_ops = pdb_heuristic.get_relevant_operators();
                assert(used_ops.size() == op_costs.size());
                for (size_t k = 0; k < used_ops.size(); ++k) {
                    if (used_ops[k])
                        op_costs[k] = 0;
                }
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
                pattern_to_fitness.insert(make_pair(modified_bitvector, mean_h));
            } else {
                mean_h = it->second;
            }
            fitness += mean_h;
        }
        fitness_values.push_back(make_pair(fitness, i));
        total_sum += fitness;
    }
    sort(fitness_values.begin(), fitness_values.end());
    return total_sum;
}

void PatternGenerationEdelkamp::bin_packing() {
    vector<int> variables;
    variables.reserve(g_variable_domain.size());
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        variables.push_back(i);
    }

    vector<int> op_costs;
    op_costs.reserve(g_operators.size());
    for (size_t i = 0; i < g_operators.size(); ++i)
        op_costs.push_back(get_adjusted_cost(g_operators[i]));

    operator_costs.reserve(num_collections);
    for (size_t i = 0; i < num_collections; ++i) {
        operator_costs.push_back(op_costs);
        random_shuffle(variables.begin(), variables.end(), g_rng);
        vector<vector<bool> > pattern_collection;
        vector<bool> pattern(g_variable_name.size(), false);
        size_t current_size = 1;
        for (size_t j = 0; j < variables.size(); ++j) {
            int var = variables[j];
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

void PatternGenerationEdelkamp::genetic_algorithm() {
    bin_packing();
    cout << "initial pattern collections:" << endl;
    dump();
    vector<pair<double, int> > initial_fitness_values;
    evaluate(initial_fitness_values);

    double current_best_h = initial_fitness_values.back().first;
    cout << "Initial best fitness value is " << current_best_h;
    vector<vector<bool> > best_collection = pattern_collections[initial_fitness_values.back().second];

    for (int t = 0; t < num_episodes; ++t) {
        cout << endl;
        cout << "--------- episode no " << t << " ---------" << endl;
        mutate();
        //cout << "current pattern_collections after mutation" << endl;
        //dump();
        vector<pair<double, int> > fitness_values;
        double fitness_sum = evaluate(fitness_values);
        cout << "evaluated" << endl;
        cout << "fitness values:";
        for (size_t i = 0; i < fitness_values.size(); ++i) {
            cout << " " << fitness_values[i].first << " (index: " << fitness_values[i].second << ")";
        }
        cout << endl;
        double new_best_h = fitness_values.back().first; // fitness_values is sorted
        /*if (new_best_h == 0.001) {
            cout << "All pattern collections are invalid. Don't select any of them.";
        } else {
            select(fitness_values, fitness_sum);
        }*/
        select(fitness_values, fitness_sum); // we allow to select invalid pattern collections
        //cout << "current pattern collections (after selection):" << endl;
        //dump();
        // the global best pattern collection is stored
        if (new_best_h > current_best_h) {
            current_best_h = new_best_h;
            best_collection = pattern_collections[fitness_values.back().second];
        }
    }

    // store patterns of the best pattern collection for faster access during search
    Options opts;
    opts.set<int>("cost_type", cost_type);
    for (size_t j = 0; j < best_collection.size(); ++j) {
        vector<int> pattern;
        for (size_t i = 0; i < best_collection[j].size(); ++i) {
            if (best_collection[j][i])
                pattern.push_back(i);
        }
        if (pattern.empty())
            continue;
        opts.set<vector<int> >("pattern", pattern);
        final_pattern_collection.push_back(new PDBHeuristic(opts, false));
    }
}

void PatternGenerationEdelkamp::initialize() {
}

int PatternGenerationEdelkamp::compute_heuristic(const State &state) {
    int h_val = 0;
    for (size_t i = 0; i < final_pattern_collection.size(); ++i) {
        final_pattern_collection[i]->evaluate(state);
        if (final_pattern_collection[i]->is_dead_end())
            return -1;
        h_val += final_pattern_collection[i]->get_heuristic();
    }
    return h_val;
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

static ScalarEvaluator *_parse(OptionParser &parser) {
    // TODO: check if 100 is the correct default value!
    parser.add_option<int>("pdb_max_size", 100, "max number of states per pdb");
    parser.add_option<int>("num_collections", 5, "number of pattern collections to maintain");
    parser.add_option<int>("num_episodes", 30, "number of episodes");
    parser.add_option<int>("mutation_probability", 1, "probability in percent for flipping a bit");
    parser.add_option<bool>("disjoint", false, "using disjoint variables in the patterns of a collection");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (opts.get<int>("pdb_max_size") < 1)
        parser.error("size per pdb must be at least 1");
    if (opts.get<int>("num_collections") < 1)
        parser.error("number of pattern collections must be at least 1");
    if (opts.get<int>("num_episodes") < 1)
        parser.error("number of episodes must be at least 1");
    if (opts.get<int>("mutation_probability") < 0 || opts.get<int>("mutation_probability") > 100)
        parser.error("mutation probability must be in [0..100]");

    if (parser.dry_run())
        return 0;

    return new PatternGenerationEdelkamp(opts);
}

static Plugin<ScalarEvaluator> _plugin("gapdb", _parse);
