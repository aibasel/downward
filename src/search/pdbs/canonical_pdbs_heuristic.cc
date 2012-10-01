#include "canonical_pdbs_heuristic.h"

#include "max_cliques.h"
#include "pdb_heuristic.h"
#include "util.h"

#include "../globals.h"
#include "../operator.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state.h"
#include "../timer.h"
#include "../utilities.h"

#include <cassert>
#include <cstdlib>
#include <vector>

using namespace std;

CanonicalPDBsHeuristic::CanonicalPDBsHeuristic(const Options &opts)
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

CanonicalPDBsHeuristic::~CanonicalPDBsHeuristic() {
    for (size_t i = 0; i < pattern_databases.size(); ++i) {
        delete pattern_databases[i];
    }
}

void CanonicalPDBsHeuristic::_add_pattern(const vector<int> &pattern) {
    Options opts;
    opts.set<int>("cost_type", cost_type);
    opts.set<vector<int> >("pattern", pattern);
    pattern_databases.push_back(new PDBHeuristic(opts, false));
    size += pattern_databases.back()->get_size();
}

bool CanonicalPDBsHeuristic::are_patterns_additive(const vector<int> &patt1,
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

void CanonicalPDBsHeuristic::compute_max_cliques() {
    // initialize compatibility graph
    max_cliques.clear();
    max_clique_indeces.clear();
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

    ::compute_max_cliques(cgraph, max_clique_indeces);

    max_cliques.reserve(max_clique_indeces.size());

    for (size_t i = 0; i < max_clique_indeces.size(); ++i) {
        vector<PDBHeuristic *> clique;
        clique.reserve(max_clique_indeces[i].size());
        for (size_t j = 0; j < max_clique_indeces[i].size(); ++j) {
            clique.push_back(pattern_databases[max_clique_indeces[i][j]]);
        }
        max_cliques.push_back(clique);
    }
}

void CanonicalPDBsHeuristic::compute_additive_vars() {
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

void CanonicalPDBsHeuristic::dominance_pruning() {
    int num_patterns = pattern_databases.size();
    int num_cliques = max_cliques.size();

    // Precompute superset relation of patterns.
    // Patters are copied and sorted for the superset test to avoid changing the
    // order in the original pattern.
    vector<vector<int> > sorted_patterns(num_patterns);
    for (size_t i = 0; i < num_patterns; ++i) {
        sorted_patterns[i] = pattern_databases[i]->get_pattern();
        std::sort(sorted_patterns[i].begin(), sorted_patterns[i].end());
    }
    vector<vector<bool> > is_superset(num_patterns, vector<bool>(num_patterns, false));
    for (size_t i = 0; i < num_patterns; ++i) {
        for (size_t j = 0; j < num_patterns; ++j) {
            if (std::includes(sorted_patterns[i].begin(),
                              sorted_patterns[i].end(),
                              sorted_patterns[j].begin(),
                              sorted_patterns[j].end())) {
                is_superset[i][j] = true;
            }
        }
    }

    // Check all pairs of cliques for dominance.
    vector<bool> clique_useful(num_cliques, false);
    int num_useful_cliques = 0;
    vector<bool> pattern_useful(num_patterns, false);
    int num_useful_patterns = 0;
    for (size_t c1_id = 0; c1_id < num_cliques; ++c1_id) {
        vector<int> &c1 = max_clique_indeces[c1_id];
        // Clique c1 is useful if it is not dominated by any clique c2.
        // Assume that it is useful and set it to false if any dominating
        // clique is found.
        bool c1_is_useful = true;
        for (size_t c2_id = 0; c2_id < num_cliques; ++c2_id) {
            if (c1_id == c2_id) {
                continue;
            }
            vector<int> &c2 = max_clique_indeces[c2_id];
            // c2 dominates c1 iff for every pattern p1 in c1 there is a pattern
            // p2 in c2 where p2 is a superset of p1.
            // Assume dominance until we fund a pattern p1 that is not dominated.
            bool c2_dominates_c1 = true;
            for (size_t pdb1_id = 0; pdb1_id < c1.size(); ++pdb1_id) {
                // Assume there is no superset until we found one.
                bool p1_has_superset = false;
                for (size_t pdb2_id = 0; pdb2_id < c2.size(); ++pdb2_id) {
                    if (is_superset[c2[pdb2_id]][c1[pdb1_id]]) {
                        p1_has_superset = true;
                        break;
                    }
                }
                if (!p1_has_superset) {
                    c2_dominates_c1 = false;
                    break;
                }
            }
            if (c2_dominates_c1) {
                c1_is_useful = false;
                break;
            }
        }
        if (c1_is_useful) {
            clique_useful[c1_id] = true;
            ++num_useful_cliques;
            for (size_t p_id = 0; p_id < c1.size(); ++p_id) {
                if (!pattern_useful[c1[p_id]]) {
                    ++num_useful_patterns;
                }
                pattern_useful[c1[p_id]] = true;
            }
        }
    }

    // Prune PDBHeuristics (note that the indices might change).
    vector<PDBHeuristic *> remaining_heuristics;
    remaining_heuristics.reserve(num_useful_patterns);
    vector<int> index_translation(num_patterns, -1);
    for (size_t i = 0; i < num_patterns; ++i) {
        if (pattern_useful[i]) {
            remaining_heuristics.push_back(pattern_databases[i]);
            index_translation[i] = remaining_heuristics.size() - 1;
        } else {
            size -= pattern_databases[i]->get_size();
        }
    }
    assert(remaining_heuristics.size() == num_useful_patterns);
    pattern_databases.swap(remaining_heuristics);

    // Prune cliques and use the new indices of the PDBHeuristics.
    vector<vector<int> > remaining_clique_indices(num_useful_cliques);
    max_cliques.clear();
    max_cliques.resize(num_useful_cliques);
    int new_clique_id = 0;
    for (size_t c_id = 0; c_id < max_clique_indeces.size(); ++c_id) {
        vector<int> &clique = max_clique_indeces[c_id];
        if (!clique_useful[c_id]) {
            continue;
        }
        // We can use swap here because clique_indices will be destroyed
        // after this loop. So we first translate the pattern indices to
        // new indices in place.
        for (size_t p_id = 0; p_id < clique.size(); ++p_id) {
            // All patterns in useful cliques are marked as useful and should
            // thus have an index translation.
            assert(index_translation[clique[p_id]] != -1);
            clique[p_id] = index_translation[clique[p_id]];
            max_cliques[new_clique_id].push_back(pattern_databases[clique[p_id]]);
        }
        remaining_clique_indices[new_clique_id].swap(clique);
        ++new_clique_id;
    }
    assert(new_clique_id == num_useful_cliques);
    max_clique_indeces.swap(remaining_clique_indices);

    cout << "Pruned " << num_cliques - num_useful_cliques <<
            " of " << num_cliques << " cliques" << endl;
    cout << "Pruned " << num_patterns - num_useful_patterns <<
            " of " << num_patterns << " PDBs" << endl;
}

void CanonicalPDBsHeuristic::initialize() {
}

int CanonicalPDBsHeuristic::compute_heuristic(const State &state) {
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

void CanonicalPDBsHeuristic::add_pattern(const vector<int> &pattern) {
    _add_pattern(pattern);
    compute_max_cliques();
}


void CanonicalPDBsHeuristic::get_max_additive_subsets(
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

void CanonicalPDBsHeuristic::dump_cgraph(const vector<vector<int> > &cgraph) const {
    // print compatibility graph
    cout << "Compatibility graph" << endl;
    for (size_t i = 0; i < cgraph.size(); ++i) {
        cout << i << " adjacent to: ";
        cout << cgraph[i] << endl;
    }
}

void CanonicalPDBsHeuristic::dump_cliques() const {
    // print maximal cliques
    assert(!max_cliques.empty());
    cout << max_cliques.size() << " maximal clique(s)" << endl;
    cout << "Maximal cliques are (";
    for (size_t i = 0; i < max_cliques.size(); ++i) {
        cout << "[";
        for (size_t j = 0; j < max_cliques[i].size(); ++j) {
            vector<int> pattern = max_cliques[i][j]->get_pattern();
            cout << "{ ";
            for (size_t k = 0; k < pattern.size(); ++k) {
                cout << pattern[k] << " ";
            }
            cout << "}";
        }
        cout << "]";
    }
    cout << ")" << endl;
}

void CanonicalPDBsHeuristic::dump() const {
    for (size_t i = 0; i < pattern_databases.size(); ++i) {
        cout << pattern_databases[i]->get_pattern() << endl;
    }
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    Options opts;
    parse_patterns(parser, opts);

    if (parser.dry_run())
        return 0;

    return new CanonicalPDBsHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cpdbs", _parse);
