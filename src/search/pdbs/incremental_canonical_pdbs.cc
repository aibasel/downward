#include "incremental_canonical_pdbs.h"

#include "dominance_pruner.h"
#include "max_cliques.h"
#include "pattern_database.h"
#include "util.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../timer.h"
#include "../utilities.h"

#include <cassert>
#include <cstdlib>
#include <vector>

using namespace std;

IncrementalCanonicalPDBs::IncrementalCanonicalPDBs(const std::shared_ptr<AbstractTask> task,
                                                   const Patterns &intitial_patterns)
    : task(task),
      task_proxy(*task) {
    Timer timer;
    size = 0;
    pattern_databases->reserve(intitial_patterns.size());
    for (const Pattern &pattern : intitial_patterns)
        add_pdb_for_pattern(pattern);
    compute_additive_vars();
    compute_max_cliques();
    cout << "PDB collection construction time: " << timer << endl;
}

void IncrementalCanonicalPDBs::add_pdb_for_pattern(const Pattern &pattern) {
    pattern_databases->push_back(make_shared<PatternDatabase>(task_proxy, pattern));
    size += pattern_databases->back()->get_size();
}

bool IncrementalCanonicalPDBs::are_patterns_additive(
    const vector<int> &pattern1, const vector<int> &pattern2) const {
    for (int v1 : pattern1) {
        for (int v2 : pattern2) {
            if (!are_additive[v1][v2]) {
                return false;
            }
        }
    }
    return true;
}

void IncrementalCanonicalPDBs::compute_max_cliques() {
    // Initialize compatibility graph.
    max_cliques->clear();
    vector<vector<int>> cgraph;
    cgraph.resize(pattern_databases->size());

    for (size_t i = 0; i < pattern_databases->size(); ++i) {
        for (size_t j = i + 1; j < pattern_databases->size(); ++j) {
            if (are_patterns_additive((*pattern_databases)[i]->get_pattern(),
                                      (*pattern_databases)[j]->get_pattern())) {
                /* If the two patterns are additive, there is an edge in the
                   compatibility graph. */
                cgraph[i].push_back(j);
                cgraph[j].push_back(i);
            }
        }
    }

    vector<vector<int>> cgraph_max_cliques;
    ::compute_max_cliques(cgraph, cgraph_max_cliques);
    max_cliques->reserve(cgraph_max_cliques.size());

    for (const vector<int> &cgraph_max_clique : cgraph_max_cliques) {
        PDBCollection clique;
        clique.reserve(cgraph_max_clique.size());
        for (int pdb_id : cgraph_max_clique) {
            clique.push_back((*pattern_databases)[pdb_id]);
        }
        max_cliques->push_back(clique);
    }
}

void IncrementalCanonicalPDBs::compute_additive_vars() {
    assert(are_additive.empty());
    int num_vars = task_proxy.get_variables().size();
    are_additive.resize(num_vars, vector<bool>(num_vars, true));
    for (OperatorProxy op : task_proxy.get_operators()) {
        for (EffectProxy e1 : op.get_effects()) {
            for (EffectProxy e2 : op.get_effects()) {
                int e1_var_id = e1.get_fact().get_variable().get_id();
                int e2_var_id = e2.get_fact().get_variable().get_id();
                are_additive[e1_var_id][e2_var_id] = false;
            }
        }
    }
}


int IncrementalCanonicalPDBs::compute_heuristic(const State &state) const {
    // If we have an empty collection, then max_cliques = { \emptyset }.
    assert(!max_cliques->empty());
    int max_h = 0;
    for (const auto &clique : *max_cliques) {
        int clique_h = 0;
        for (const auto &pdb : clique) {
            /* Experiments showed that it is faster to recompute the
               h values than to cache them in an unordered_map. */
            int h = pdb->get_value(state);
            if (h == numeric_limits<int>::max())
                return numeric_limits<int>::max();
            clique_h += h;
        }
        max_h = max(max_h, clique_h);
    }
    return max_h;
}

void IncrementalCanonicalPDBs::add_pattern(const vector<int> &pattern) {
    add_pdb_for_pattern(pattern);
    compute_max_cliques();
}


void IncrementalCanonicalPDBs::get_max_additive_subsets(
    const Pattern &new_pattern,
    PDBCliques &max_additive_subsets) {
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

    for (const auto &clique : *max_cliques) {
        // Take all patterns which are additive to new_pattern.
        PDBCollection subset;
        subset.reserve(clique.size());
        for (const auto &pdb : clique) {
            if (are_patterns_additive(new_pattern, pdb->get_pattern())) {
                subset.push_back(pdb);
            }
        }
        if (!subset.empty()) {
            max_additive_subsets.push_back(subset);
        }
    }
    if (max_additive_subsets.empty()) {
        // If nothing was additive with the new variable, then
        // the only additive subset is the empty set.
        max_additive_subsets.emplace_back();
    }
}

bool IncrementalCanonicalPDBs::is_dead_end(const State &state) const {
    for (const auto &pdb : *pattern_databases)
        if (pdb->get_value(state) == numeric_limits<int>::max())
            return true;
    return false;
}

