#include "pdb_collection_heuristic.h"
#include "globals.h"
#include "plugin.h"
#include "state.h"
#include "operator.h"
#include "timer.h"
#include "planning_utilities.h"

#include <cstdlib>
#include <limits>
#include <cassert>

using namespace std;

static ScalarEvaluatorPlugin pdbcollection_heuristic_plugin("pdbs", PDBCollectionHeuristic::create);

PDBCollectionHeuristic::PDBCollectionHeuristic() {
}

PDBCollectionHeuristic::~PDBCollectionHeuristic() {
}

void PDBCollectionHeuristic::add_new_pattern(const vector<int> &pattern, PDBHeuristic *pdb) {
    // TODO only pdb-abstraction and no more ints
    pattern_collection.push_back(pattern);
    pattern_databases.push_back(pdb);
    max_cliques.clear();
    precompute_max_cliques();
}

void PDBCollectionHeuristic::get_max_additive_subsets(const vector<int> &new_pattern,
                                                      vector<vector<int> > &max_additive_subsets) {
    for (size_t i = 0; i < max_cliques.size(); ++i) {
        // take all patterns which are additive to new_pattern
        vector<int> subset;
        subset.reserve(max_cliques[i].size());
        for (size_t j = 0; j < max_cliques[i].size(); ++j) {
            if (are_pattern_additive(new_pattern, pattern_collection[max_cliques[i][j]])) {
                subset.push_back(max_cliques[i][j]);
            }
        }
        if (subset.size() > 0) {
            max_additive_subsets.push_back(subset);
        }
    }
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
    cgraph.resize(pattern_collection.size());

    for (size_t i = 0; i < pattern_collection.size(); ++i) {
        for (size_t j = i + 1; j < pattern_collection.size(); ++j) {
            if (are_pattern_additive(pattern_collection[i],pattern_collection[j])) {
                // if the two patterns are additive there is an edge in the compatibility graph
                cgraph[i].push_back(j);
                cgraph[j].push_back(i);
            }
        }
    }
    cout << "built cgraph." << endl;

    compute_max_cliques(cgraph, max_cliques);

    dump(cgraph);
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

   /*cout << "are_additive_matrix" << endl;
   for (int i = 0; i < are_additive.size(); ++i) {
        cout << i << ": ";
        for (int j = 0; j < are_additive[i].size(); ++j) {
            cout << are_additive[i][j] << " ";
        }
        cout << endl;
   }*/
}

void PDBCollectionHeuristic::initialize() {
    cout << "Initializing pattern database heuristic..." << endl;

    // Simple selection strategy. Take all goal variables as patterns.
    for (size_t i = 0; i < g_goal.size(); ++i) {
        pattern_collection.push_back(vector<int>(1, g_goal[i].first));
    }

    precompute_additive_vars();
    precompute_max_cliques();

    // build all pattern databases
    Timer timer;
    for (int i = 0; i < pattern_collection.size(); ++i) {
        pattern_databases.push_back(new PDBHeuristic(pattern_collection[i]));
    }
    cout << pattern_collection.size() << " pdbs constructed." << endl;
    cout << "Construction time for all pdbs: " << timer << endl;

    // testing logistics 6-2
    /*vector<int> test_pattern;
    test_pattern.push_back(2);
    test_pattern.push_back(7);
    vector<vector<int> > max_add_sub;
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

    add_new_pattern(test_pattern, new PDBHeuristic(test_pattern));*/
}

int PDBCollectionHeuristic::compute_heuristic(const State &state) {
    int max_val = -1;
    for (size_t i = 0; i < max_cliques.size(); ++i) {
        const vector<int> &clique = max_cliques[i];
        int h_val = 0;
        for (size_t j = 0; j < clique.size(); ++j) {
            pattern_databases[clique[j]]->evaluate(state);
            int h = pattern_databases[clique[j]]->get_heuristic();
            if (h == numeric_limits<int>::max()) {
                return -1;
            }
            h_val += h;
        }
        if (h_val > max_val) {
            max_val = h_val;
        }
    }
    return max_val;
    //TODO check if this is correct!
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
    cout << "Maximal cliques are { ";
    for (size_t i = 0; i < max_cliques.size(); ++i) {
        cout << "[ ";
        for (size_t j = 0; j < max_cliques[i].size(); ++j) {
            cout << max_cliques[i][j] << " ";
        }
        cout << "] ";
    }
    cout << "}" << endl;
}

ScalarEvaluator *PDBCollectionHeuristic::create(const vector<string> &config, int start, int &end, bool dry_run) {
    //TODO: check what we have to do here!
    OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    else
        return new PDBCollectionHeuristic;
}

