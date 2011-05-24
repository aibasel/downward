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
#include <ext/hash_set>

using namespace __gnu_cxx;
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

void PatternGenerationEdelkamp::select(const vector<pair<double, int> > &fitness_values) {
    vector<double> cumulative_fitness;
    cumulative_fitness.reserve(fitness_values.size());
    double total_so_far = 0;
    for (size_t i = 0; i < fitness_values.size(); ++i) {
        total_so_far += fitness_values[i].first;
        cumulative_fitness.push_back(total_so_far);
    }
    // total_so_far is now sum over all fitness values

    vector<vector<vector<bool> > > new_pattern_collections;
    new_pattern_collections.reserve(num_collections);
    for (size_t i = 0; i < num_collections; ++i) {
        int selected;
        if (total_so_far == 0) {
            // All fitness values are 0 => choose uniformly.
            selected = g_rng(fitness_values.size());
        } else {
            double random = g_rng() * total_so_far; // [0..total_so_far)
            // Find first entry which is strictly greater than random.
            selected = upper_bound(cumulative_fitness.begin(),
                                   cumulative_fitness.end(), random) -
                cumulative_fitness.begin();
        }
        new_pattern_collections.push_back(pattern_collections[fitness_values[selected].second]);
    }
    pattern_collections.swap(new_pattern_collections);
}

void PatternGenerationEdelkamp::mutate() {
    for (size_t i = 0; i < pattern_collections.size(); ++i) {
        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            vector<bool> &pattern = pattern_collections[i][j];
            for (size_t k = 0; k < pattern.size(); ++k) {
                double random = g_rng(); // [0..1)
                if (random < mutation_probability) {
                    pattern[k].flip();
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

void PatternGenerationEdelkamp::remove_irrelevant_variables(
    vector<int> &pattern) const {
    hash_set<int> in_original_pattern(pattern.begin(), pattern.end());
    hash_set<int> in_pruned_pattern;

    vector<int> vars_to_check;
    for (size_t i = 0; i < g_goal.size(); ++i) {
        int var_no = g_goal[i].first;
        if (in_original_pattern.count(var_no)) {
            // Goals are causally relevant.
            vars_to_check.push_back(var_no);
            in_pruned_pattern.insert(var_no);
        }
    }

    while(!vars_to_check.empty()) {
        int var = vars_to_check.back();
        vars_to_check.pop_back();
        const vector<int> &rel = g_causal_graph->get_predecessors(var);
        for (size_t i = 0; i < rel.size(); ++i) {
            int var_no = rel[i];
            if (in_original_pattern.count(var_no) &&
                !in_pruned_pattern.count(var_no)) {
                // Parents of relevant variables are causally relevant.
                vars_to_check.push_back(var_no);
                in_pruned_pattern.insert(var_no);
            }
        }
    }

    pattern.assign(in_pruned_pattern.begin(), in_pruned_pattern.end());
}

bool PatternGenerationEdelkamp::is_pattern_too_large(
    const vector<int> &pattern) const {
    // test if the pattern respects the memory limit
    int mem = 1;
    for (size_t i = 0; i < pattern.size(); ++i) {
        int domain_size = g_variable_domain[pattern[i]];
        // test against overflow and pdb_max_size
        if (mem > pdb_max_size / domain_size)
            return true;
        mem *= domain_size;
    }
    return false;
}

bool PatternGenerationEdelkamp::mark_used_variables(
    const vector<int> &pattern, vector<bool> &variables_used) const {
    for (size_t i = 0; i < pattern.size(); ++i) {
        int var_no = pattern[i];
        if (variables_used[var_no])
            return true;
        variables_used[var_no] = true;
    }
    return false;
}

void PatternGenerationEdelkamp::evaluate(vector<pair<double, int> > &fitness_values) {
    for (size_t i = 0; i < pattern_collections.size(); ++i) {
        cout << "evaluate pattern collection " << i << " of " << (pattern_collections.size() - 1) << endl;
        double fitness = 0;
        vector<bool> variables_used(g_variable_domain.size(), false);
        vector<int> op_costs = operator_costs[i]; // operator costs for actual pattern collection (must be copied!)

        for (size_t j = 0; j < pattern_collections[i].size(); ++j) {
            const vector<bool> &bitvector = pattern_collections[i][j];
            vector<int> pattern;
            transform_to_pattern_normal_form(bitvector, pattern);

            if (is_pattern_too_large(pattern)) {
                cout << "pattern " << j << " exceeds the memory limit!" << endl;
                fitness = 0.001;
                break;
            }

            if (disjoint_patterns) {
                if (mark_used_variables(pattern, variables_used)) {
                    cout << "patterns are not disjoint anymore!" << endl;
                    fitness = 0.001;
                    break;
                }
            }

            remove_irrelevant_variables(pattern);

            // calculate mean h-value for actual pattern collection
            double mean_h = 0;

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
            fitness += mean_h;
        }
        fitness_values.push_back(make_pair(fitness, i));
    }
    sort(fitness_values.begin(), fitness_values.end());
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
        // initally, all pattern collections use default operator costs. action cost partitioning
        // comes into play after construction the first PDBs.
        operator_costs.push_back(op_costs);
        // random variable ordering for all pattern collections
        random_shuffle(variables.begin(), variables.end(), g_rng);
        vector<vector<bool> > pattern_collection;
        vector<bool> pattern(g_variable_name.size(), false);
        size_t current_size = 1;
        for (size_t j = 0; j < variables.size(); ++j) {
            int var = variables[j];
            int next_var_size = g_variable_domain[var];
            if (next_var_size <= pdb_max_size) {
                // test against overflow and pdb_max_size
                if (current_size > pdb_max_size / next_var_size) {
                    // current_size * next_var_size > pdb_max_size
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
        evaluate(fitness_values);
        cout << "evaluated" << endl;
        /*cout << "fitness values:";
        for (size_t i = 0; i < fitness_values.size(); ++i) {
            cout << " " << fitness_values[i].first << " (index: " << fitness_values[i].second << ")";
        }
        cout << endl;*/
        double new_best_h = fitness_values.back().first; // fitness_values is sorted
        select(fitness_values); // we allow to select invalid pattern collections
        //cout << "current pattern collections (after selection):" << endl;
        //dump();
        // the overall best pattern collection of all episodes is stored
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
    // since we use action cost partitioning, we can simply add up all h-values
    // from the patterns in the pattern collection
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
