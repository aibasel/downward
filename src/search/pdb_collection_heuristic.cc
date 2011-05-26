#include "pdb_collection_heuristic.h"
#include "globals.h"
#include "max_cliques.h"
#include "operator.h"
#include "option_parser.h"
#include "pdb_heuristic.h"
#include "plugin.h"
#include "state.h"
#include "timer.h"

#include <cassert>
#include <cstdlib>
#include <vector>

using namespace std;

PDBCollectionHeuristic::PDBCollectionHeuristic(const Options &opts)
    : Heuristic(opts) {
    const vector<vector<int> > &pattern_collection(opts.get_list<vector<int> >("patterns"));
    Timer timer;
    size = 0;
    pattern_databases.reserve(pattern_collection.size());
    for (size_t i = 0; i < pattern_collection.size(); ++i)
        _add_pattern(pattern_collection[i]);
    compute_additive_vars();
    compute_max_cliques();
    cout << "PDB collection construction time: " << timer << endl;
}

PDBCollectionHeuristic::~PDBCollectionHeuristic() {
    for (size_t i = 0; i < pattern_databases.size(); ++i) {
        delete pattern_databases[i];
    }
}

void PDBCollectionHeuristic::_add_pattern(const vector<int> &pattern) {
    Options opts;
    opts.set<int>("cost_type", cost_type);
    opts.set<vector<int> >("pattern", pattern);
    pattern_databases.push_back(new PDBHeuristic(opts, false));
    size += pattern_databases.back()->get_size();
}

bool PDBCollectionHeuristic::are_patterns_additive(const vector<int> &patt1,
                                                  const vector<int> &patt2) const {
    for (size_t i = 0; i < patt1.size(); ++i) {
        for (size_t j = 0; j < patt2.size(); ++j) {
            if (!are_additive[patt1[i]][patt2[j]]) {
                return false;
            }
        }
    }
    return true;
}

void PDBCollectionHeuristic::compute_max_cliques() {
    // initialize compatibility graph
    max_cliques.clear();
    vector<vector<int> > cgraph;
    cgraph.resize(pattern_databases.size());

    for (size_t i = 0; i < pattern_databases.size(); ++i) {
        for (size_t j = i + 1; j < pattern_databases.size(); ++j) {
            if (are_patterns_additive(pattern_databases[i]->get_pattern(),
                                     pattern_databases[j]->get_pattern())) {
                // if the two patterns are additive there is an edge in the compatibility graph
                cgraph[i].push_back(j);
                cgraph[j].push_back(i);
            }
        }
    }
    //cout << "built cgraph." << endl;

    vector<vector<int> > cgraph_max_cliques;
    ::compute_max_cliques(cgraph, cgraph_max_cliques);
    max_cliques.reserve(cgraph_max_cliques.size());
    
    for (size_t i = 0; i < cgraph_max_cliques.size(); ++i) {
        vector<PDBHeuristic *> clique;
        clique.reserve(cgraph_max_cliques[i].size());
        for (size_t j = 0; j < cgraph_max_cliques[i].size(); ++j) {
            clique.push_back(pattern_databases[cgraph_max_cliques[i][j]]);
        }
        max_cliques.push_back(clique);
    }

    //dump(cgraph);
}

void PDBCollectionHeuristic::compute_additive_vars() {
    assert(are_additive.empty());
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
}

void PDBCollectionHeuristic::initialize() {
}

int PDBCollectionHeuristic::compute_heuristic(const State &state) {
    int max_h = 0;
    assert(!max_cliques.empty());
    // if we have an empty collection, then max_cliques = { \emptyset }
    for (size_t i = 0; i < max_cliques.size(); ++i) {
        const vector<PDBHeuristic *> &clique = max_cliques[i];
        int clique_h = 0;
        for (size_t j = 0; j < clique.size(); ++j) {
            clique[j]->evaluate(state);
            if (clique[j]->is_dead_end())
                return -1;
            clique_h += clique[j]->get_heuristic();
        }
        max_h = max(max_h, clique_h);
    }
    return max_h;
}

void PDBCollectionHeuristic::add_pattern(const vector<int> &pattern) {
    add_pattern(pattern);
    compute_max_cliques();
}


void PDBCollectionHeuristic::get_max_additive_subsets(
    const vector<int> &new_pattern, vector<vector<PDBHeuristic *> > &max_additive_subsets) {
    /*
      We compute additive pattern sets S with the property that we could
      add the new pattern P to S and still have an additive pattern set.

      Ideally, we would like to return all *maximal* sets S with this
      property (w.r.t. set inclusion), but we don't currently
      guarantee this. (What we guarantee is that all maximal such sets
      are *included* in the result, but the result could contain
      duplicates or sets that are subsets of other sets in the
      result.)

      We currently implement this as follows:

      * Consider all maximal additive subsets of the current collection.
      * For each additive subset S, take the subset S' that contains
        those patterns that are additive with the new pattern P.
      * Include the subset S' in the result.

      As an optimization, we actually only include S' in the result if
      it is non-empty. However, this is wrong if *all* subsets we get
      are empty, so we correct for this case at the end.

      This may include dominated elements and duplicates in the result.
      To avoid this, we could instead use the following algorithm:

      * Let N (= neighbours) be the set of patterns in our current
        collection that are additive with the new pattern P.
      * Let G_N be the compatibility graph of the current collection
        restricted to set N (i.e. drop all non-neighbours and their
        incident edges.)
      * Return the maximal cliques of G_N.

      One nice thing about this alternative algorithm is that we could
      also use it to incrementally compute the new set of maximal additive
      pattern sets after adding the new pattern P:

      G_N_cliques = max_cliques(G_N)   // as above
      new_max_cliques = (old_max_cliques \setminus G_N_cliques) \union
                        { clique \union {P} | clique in G_N_cliques}

      That is, the new set of maximal cliques is exactly the set of
      those "old" cliques that we cannot extend by P
      (old_max_cliques \setminus G_N_cliques) and all
      "new" cliques including P.
      */

    for (size_t i = 0; i < max_cliques.size(); ++i) {
        // take all patterns which are additive to new_pattern
        vector<PDBHeuristic *> subset;
        subset.reserve(max_cliques[i].size());
        for (size_t j = 0; j < max_cliques[i].size(); ++j) {
            if (are_patterns_additive(new_pattern, max_cliques[i][j]->get_pattern())) {
                subset.push_back(max_cliques[i][j]);
            }
        }
        if (!subset.empty()) {
            max_additive_subsets.push_back(subset);
        }
    }
    if (max_additive_subsets.empty()) {
        // If nothing was additive with the new variable, then
        // the only additive subset is the empty set.
        max_additive_subsets.push_back(vector<PDBHeuristic *>());
    }
}

void PDBCollectionHeuristic::dump_cgraph(const vector<vector<int> > &cgraph) const {
    // print compatibility graph
    cout << "Compatibility graph" << endl;
    for (size_t i = 0; i < cgraph.size(); ++i) {
        cout << i << " adjacent to [ ";
        for (size_t j = 0; j < cgraph[i].size(); ++j) {
            cout << cgraph[i][j] << " ";
        }
        cout << "]" << endl;
    }
}

void PDBCollectionHeuristic::dump() const {
    // print maximal cliques
    assert(!max_cliques.empty());
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
    // TODO: Distinguish the case that no collection was specified by
    // the user from the case that an empty collection was explicitly
    // specified by the user (see issue236).
    if (parser.dry_run() && !pattern_collection.empty()) {
        // check if there are duplicates of patterns
        for (size_t i = 0; i < pattern_collection.size(); ++i) {
            sort(pattern_collection[i].begin(), pattern_collection[i].end());
            // check if each pattern is valid
            int pat_old_size = pattern_collection[i].size();
            vector<int>::const_iterator it = unique(pattern_collection[i].begin(), pattern_collection[i].end());
            pattern_collection[i].resize(it - pattern_collection[i].begin());
            if (pattern_collection[i].size() != pat_old_size)
                parser.error("there are duplicates of variables in a pattern");
            if (!pattern_collection[i].empty()) {
                if (pattern_collection[i].front() < 0)
                    parser.error("there is a variable < 0 in a pattern");
                if (pattern_collection[i].back() >= g_variable_domain.size())
                    parser.error("there is a variable > number of variables in a pattern");
            }
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

    // TODO: See above (issue236).
    if (pattern_collection.empty()) {
        // Simple selection strategy. Take all goal variables as patterns.
        for (size_t i = 0; i < g_goal.size(); ++i) {
            pattern_collection.push_back(vector<int>(1, g_goal[i].first));
        }
        opts.set("patterns", pattern_collection);
    }

    return new PDBCollectionHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("pdbs", _parse);
