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
    mutation_probability(opts.get<int>("mutation_probability") / 100.0),
    disjoint_patterns(opts.get<bool>("disjoint")) {
    Timer timer;
    genetic_algorithm(opts.get<int>("num_episodes"));
    cout << "Pattern Generation (Edelkamp) time: " << timer << endl;
}

PatternGenerationEdelkamp::~PatternGenerationEdelkamp() {
    // TODO: check correctness - apparently this destructor is never called anyways
    /*cout << "destructor called" << endl;
    for (size_t i = 0; i < final_pattern_collection.size(); ++i) {
        delete final_pattern_collection[i];
    }*/
}

/* TODO: Generell alle Variablen aus den
Patterns entfernen, die nicht "kausal gerechtfertigt" sind, d.h.

* Zielvariablen sind, oder
* in Vorbedingungen von Operatoren auftauchen, die bereits
kausal gerechtfertigte Variablen in den Effekten haben

Damit bekäme man kleinere Patterns und vermutlich auch mehr Duplikate,
d.h. Patterns, wo man schon auf gecachete Kosten zurückgreifen kann.
*/

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

void PatternGenerationEdelkamp::mutate() {
    // TODO: Should we check the max pdb size here? After mutation new variables can occur in a pattern and
    // exceed the max_pdb_size!
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
                if (random < mutation_probability) {
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
                if (bitvector[k] == 1)
                    mem *= g_variable_domain[k];
            }
            if (mem > pdb_max_size) {
                cout << "pattern " << j << " exceeds the memory limit!" << endl;
                fitness = 0.001;
                break;
            }

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


            /*vector<bool> modified_bitvector;
            modified_bitvector.resize(bitvector.size());
            // try to remove irrelevant variables (temporary)
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
            }*/

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
            map<vector<bool>, double>::const_iterator it = pattern_to_fitness.find(bitvector);
            double mean_h = 0;
            if (it == pattern_to_fitness.end()) {
                vector<int> pattern;
                transform_to_pattern_normal_form(bitvector, pattern);
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

                PDBHeuristic pdb_heuristic(pattern, false, op_costs);

                // at the very first time, op_costs is empty and gets initialized in pdb_heuristic
                // as there is no way to precompute the operator costs in an elegant way as in pdb_heuristic, because
                // this class doesn't inherit from Heuristic, we just get the operator costs from pdb_heuristic
                //if (op_costs.empty()) 
                    //op_costs = pdb_heuristic.get_op_costs();

                // get used operators and set their cost for further iterations to 0 (action cost partitioning)
                const vector<bool> &used_ops = pdb_heuristic.get_used_ops();
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
                pattern_to_fitness.insert(make_pair(bitvector, mean_h));
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
    for (size_t num_pcs = 0; num_pcs < num_collections; ++num_pcs) {
        operator_costs.push_back(op_costs);
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

void PatternGenerationEdelkamp::genetic_algorithm(int num_episodes) {
    bin_packing();
    //cout << "initial pattern collections:" << endl;
    //dump();
    vector<pair<double, int> > initial_fitness_values;
    evaluate(initial_fitness_values);

    double current_best_h = initial_fitness_values.back().first;
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
        /*cout << "fitness values:";
        for (size_t i = 0; i < fitness_values.size(); ++i) {
            cout << " " << fitness_values[i].first << " (index: " << fitness_values[i].second << ")";
        }
        cout << endl;*/
        double new_best_h = fitness_values.back().first;
        select(fitness_values, fitness_sum);
        //cout << "current pattern collections (after selection):" << endl;
        //dump();
        if (new_best_h > current_best_h) {
            current_best_h = new_best_h;
            best_collection = pattern_collections[fitness_values.back().second];
        }
    }

    // store patterns of the best pattern collection for faster access during search
    for (size_t j = 0; j < best_collection.size(); ++j) {
        vector<int> pattern;
        for (size_t i = 0; i < best_collection[j].size(); ++i) {
            if (best_collection[j][i])
                pattern.push_back(i);
        }
        if (pattern.empty())
            continue;
        final_pattern_collection.push_back(new PDBHeuristic(pattern, false));
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

/*PDBCollectionHeuristic *PatternGenerationEdelkamp::get_pattern_collection_heuristic() const {
    // return the best collection of the last pattern collections (after all episodes)
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
}*/

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
