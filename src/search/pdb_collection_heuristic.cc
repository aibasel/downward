#include "pdb_collection_heuristic.h"
#include "pdb_heuristic.h"
#include "globals.h"
#include "operator.h"
#include "option_parser.h"
#include "planning_utilities.h"
#include "plugin.h"
#include "state.h"
#include "timer.h"

#include <cassert>
#include <cstdlib>
#include <limits>
#include <vector>

using namespace std;

PDBCollectionHeuristic::PDBCollectionHeuristic(const Options &opts)
    : Heuristic(opts) {
    const vector<vector<int> > &pattern_collection(opts.get_list<vector<int> >("patterns"));
    Timer timer;
    for (int i = 0; i < pattern_collection.size(); ++i) {
        Options opt;
        opt.set("pattern", pattern_collection[i]);
        vector<int> pattern = opt.get_list<int>("pattern");
        pattern_databases.push_back(new PDBHeuristic(opts, vector<int>(), false));
    }
    cout << pattern_collection.size() << " pdbs constructed." << endl;
    cout << "Construction time for all pdbs: " << timer << endl;
    precompute_additive_vars();
    precompute_max_cliques();
}

PDBCollectionHeuristic::~PDBCollectionHeuristic() {
}

bool PDBCollectionHeuristic::are_pattern_additive(const vector<int> &patt1, const vector<int> &patt2) const {
    for (size_t i = 0; i < patt1.size(); ++i) {
        for (size_t j = 0; j < patt2.size(); ++j) {
            if (!are_additive[patt1[i]][patt2[j]]) {
                return false;
            }
        }
    }
    return true;
}

void PDBCollectionHeuristic::precompute_max_cliques() {
    // initialize compatibility graph
    vector<vector<int> > cgraph;
    cgraph.resize(pattern_databases.size());
    
    for (size_t i = 0; i < pattern_databases.size(); ++i) {
        for (size_t j = i + 1; j < pattern_databases.size(); ++j) {
            if (are_pattern_additive(pattern_databases[i]->get_pattern(),pattern_databases[j]->get_pattern())) {
                // if the two patterns are additive there is an edge in the compatibility graph
                cgraph[i].push_back(j);
                cgraph[j].push_back(i);
            }
        }
    }
    //cout << "built cgraph." << endl;
    
    vector<vector<int> > max_cliques_cgraph;
    max_cliques_cgraph.reserve(pattern_databases.size());
    compute_max_cliques(cgraph, max_cliques_cgraph);
    
    for (size_t i = 0; i < max_cliques_cgraph.size(); ++i) {
        vector<PDBHeuristic *> clique;
        clique.reserve(max_cliques_cgraph[i].size());
        for (size_t j = 0; j < max_cliques_cgraph[i].size(); ++j) {
            clique.push_back(pattern_databases[max_cliques_cgraph[i][j]]);
        }
        max_cliques.push_back(clique);
    }
    
    //dump(cgraph);
}

void PDBCollectionHeuristic::precompute_additive_vars() {
    int num_vars = g_variable_domain.size();
    are_additive.resize(num_vars, vector<bool>(num_vars, true));
    for (size_t k = 0; k < g_operators.size(); ++k) {
        const Operator &o = g_operators[k];
        const vector<PrePost> effects = o.get_pre_post();
        for (size_t e1 = 0; e1 < effects.size(); ++e1) {
            for (size_t e2 = 0; e2 < effects.size(); ++e2) {
                are_additive[effects[e1].var][effects[e2].var] = false;
            }
        }
    }
    /*cout << "are additive variables matrix" << endl;
    for (size_t i = 0; i < are_additive.size(); ++i) {
        for (size_t j = 0; j < are_additive[i].size(); ++j) {
            cout << are_additive[i][j] << " ";
        }
        cout << endl;
    }*/
}

void PDBCollectionHeuristic::initialize() {
    //cout << "Initializing pattern database heuristic..." << endl;
    //cout << "Didn't do anything. Done initializing." << endl;

    // testing logistics 6-2
    /*vector<int> test_pattern;
    test_pattern.push_back(2);
    test_pattern.push_back(7);
    vector<vector<PDBHeuristic *> > max_add_sub;
    get_max_additive_subsets(test_pattern, max_add_sub);
    cout << "Maximal additive subsets are { ";
    for (size_t i = 0; i < max_add_sub.size(); ++i) {
        cout << "[ ";
        for (size_t j = 0; j < max_add_sub[i].size(); ++j) {
            cout << max_add_sub[i][j] << " ";
        }
        cout << "] ";
    }
    cout << "}" << endl;

    add_new_pattern(new PDBHeuristic(test_pattern));*/
}

int PDBCollectionHeuristic::compute_heuristic(const State &state) {
    int max_val = -1;
    for (size_t i = 0; i < max_cliques.size(); ++i) {
        const vector<PDBHeuristic *> &clique = max_cliques[i];
        int h_val = 0;
        for (size_t j = 0; j < clique.size(); ++j) {
            clique[j]->evaluate(state);
            if (clique[j]->is_dead_end())
                return -1;
            int h = clique[j]->get_heuristic();
            h_val += h;
        }
        max_val = max(max_val, h_val);
    }
    return max_val;
}

void PDBCollectionHeuristic::add_new_pattern(const vector<int> &pattern) {
    Options opts;
    opts.set("pattern", pattern);
    pattern_databases.push_back(new PDBHeuristic(opts, vector<int>(), false));
    max_cliques.clear();
    precompute_max_cliques();
}

void PDBCollectionHeuristic::get_max_additive_subsets(const vector<int> &new_pattern,
                                                      vector<vector<PDBHeuristic *> > &max_additive_subsets) {
    for (size_t i = 0; i < max_cliques.size(); ++i) {
        // take all patterns which are additive to new_pattern
        vector<PDBHeuristic *> subset;
        subset.reserve(max_cliques[i].size());
        for (size_t j = 0; j < max_cliques[i].size(); ++j) {
            if (are_pattern_additive(new_pattern, max_cliques[i][j]->get_pattern())) {
                subset.push_back(max_cliques[i][j]);
            }
        }
        if (!subset.empty()) {
            max_additive_subsets.push_back(subset);
        }
    }
}

void PDBCollectionHeuristic::dump(const vector<vector<int> > &cgraph) const {
    // print compatibility graph
    cout << "Compatibility graph" << endl;
    for (size_t i = 0; i < cgraph.size(); ++i) {
        cout << i << " adjacent to [ ";
        for (size_t j = 0; j < cgraph[i].size(); ++j) {
            cout << cgraph[i][j] << " ";
        }
        cout << "]" << endl;
    }
    // print maximal cliques
    assert(max_cliques.size() > 0);
    cout << max_cliques.size() << " maximal clique(s)" << endl;
    cout << "Maximal cliques are (";
    for (size_t i = 0; i < max_cliques.size(); ++i) {
        cout << "[";
        for (size_t j = 0; j < max_cliques[i].size(); ++j) {
            vector<int> pattern = max_cliques[i][j]->get_pattern();
            cout << "{";
            for (size_t k = 0; k < pattern.size(); ++k) {
                cout << pattern[k] << " ";
            }
            cout << "}";
        }
        cout << "]";
    }
    cout << ")" << endl;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_list_option<vector<int> >("patterns", vector<vector<int> >(), "the pattern collection");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    vector<vector<int> > pattern_collection = opts.get_list<vector<int> >("patterns");
    for (size_t i = 0; i < pattern_collection.size(); ++i) {
        cout << "[ ";
        for (size_t j = 0; j < pattern_collection[i].size(); ++j) {
            cout << pattern_collection[i][j] << " ";
        }
        cout << "]" << endl;
    }
    cout << endl;
    if (parser.dry_run() && !pattern_collection.empty()) {
        // check if there are duplicates of patterns
        for (size_t i = 0; i < pattern_collection.size(); ++i) {
            sort(pattern_collection[i].begin(), pattern_collection[i].end());
            // check if each pattern is valid
            int pat_old_size = pattern_collection[i].size();
            if (pat_old_size == 0)
                parser.error("there is an empty pattern in the pattern collection");
            vector<int>::const_iterator it = unique(pattern_collection[i].begin(), pattern_collection[i].end());
            pattern_collection[i].resize(it - pattern_collection[i].begin());
            if (pattern_collection[i].size() != pat_old_size)
                parser.error("there are duplicates of variables in a pattern");
            if (pattern_collection[i][0] < 0)
                parser.error("there is a variable < 0 in a pattern");
            if (pattern_collection[i][pattern_collection[i].size() - 1] > g_variable_domain.size())
                parser.error("there is a variable > number of variables in a pattern");
        }
        sort(pattern_collection.begin(), pattern_collection.end());
        int coll_old_size = pattern_collection.size();
        vector<vector<int> >::const_iterator it = unique(pattern_collection.begin(), pattern_collection.end());
        pattern_collection.resize(it - pattern_collection.begin());
        if (pattern_collection.size() != coll_old_size)
            parser.error("there are duplicates of patterns in the pattern collection");

        for (size_t i = 0; i < pattern_collection.size(); ++i) {
            cout << "[ ";
            for (size_t j = 0; j < pattern_collection[i].size(); ++j) {
                cout << pattern_collection[i][j] << " ";
            }
            cout << "]" << endl;
        }

    }

    if (parser.dry_run())
        return 0;

    if (pattern_collection.empty()) {
        // Simple selection strategy. Take all goal variables as patterns.
        for (size_t i = 0; i < g_goal.size(); ++i) {
            pattern_collection.push_back(vector<int>(1, g_goal[i].first));
        }
        /*cout << "goals are" << endl;
        for (size_t i = 0; i < pattern_collection.size(); ++i) {
            cout << "[ ";
            for (size_t j = 0; j < pattern_collection[i].size(); ++j) {
                cout << pattern_collection[i][j] << " ";
            }
            cout << "]" << endl;
        }
        cout << endl;*/
        opts.set("patterns", pattern_collection);
    }


    return new PDBCollectionHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("pdbs", _parse);
