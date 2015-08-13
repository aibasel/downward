#include "canonical_pdbs_heuristic.h"

#include "dominance_pruner.h"
#include "max_cliques.h"
#include "pattern_database.h"
#include "util.h"

#include "../evaluation_context.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../timer.h"
#include "../utilities.h"

#include <cassert>
#include <cstdlib>
#include <unordered_map>
#include <vector>

using namespace std;

CanonicalPDBsHeuristic::CanonicalPDBsHeuristic(const Options &opts)
    : Heuristic(opts) {
    const vector<vector<int> > &pattern_collection(
        opts.get_list<vector<int> >("patterns"));
    Timer timer;
    size = 0;
    pattern_databases.reserve(pattern_collection.size());
    for (const vector<int> &pattern : pattern_collection)
        add_pdb_for_pattern(pattern);
    compute_additive_vars();
    compute_max_cliques();
    cout << "PDB collection construction time: " << timer << endl;
}

CanonicalPDBsHeuristic::~CanonicalPDBsHeuristic() {
    for (PatternDatabase *pdb : pattern_databases) {
        delete pdb;
    }
}

void CanonicalPDBsHeuristic::add_pdb_for_pattern(const vector<int> &pattern) {
    pattern_databases.push_back(new PatternDatabase(task, pattern));
    size += pattern_databases.back()->get_size();
}

bool CanonicalPDBsHeuristic::are_patterns_additive(
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

void CanonicalPDBsHeuristic::compute_max_cliques() {
    // Initialize compatibility graph.
    max_cliques.clear();
    vector<vector<int> > cgraph;
    cgraph.resize(pattern_databases.size());

    for (size_t i = 0; i < pattern_databases.size(); ++i) {
        for (size_t j = i + 1; j < pattern_databases.size(); ++j) {
            if (are_patterns_additive(pattern_databases[i]->get_pattern(),
                                      pattern_databases[j]->get_pattern())) {
                /* If the two patterns are additive, there is an edge in the
                   compatibility graph. */
                cgraph[i].push_back(j);
                cgraph[j].push_back(i);
            }
        }
    }

    vector<vector<int> > cgraph_max_cliques;
    ::compute_max_cliques(cgraph, cgraph_max_cliques);
    max_cliques.reserve(cgraph_max_cliques.size());

    for (const vector<int> &cgraph_max_clique : cgraph_max_cliques) {
        vector<PatternDatabase *> clique;
        clique.reserve(cgraph_max_clique.size());
        for (int pdb_id : cgraph_max_clique) {
            clique.push_back(pattern_databases[pdb_id]);
        }
        max_cliques.push_back(clique);
    }
}

void CanonicalPDBsHeuristic::compute_additive_vars() {
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

void CanonicalPDBsHeuristic::dominance_pruning() {
    Timer timer;
    int num_patterns = pattern_databases.size();
    int num_cliques = max_cliques.size();

    DominancePruner(pattern_databases, max_cliques).prune();

    // Recompute size (if patterns were pruned, the size is out of date).
    size = 0;
    for (PatternDatabase *pdb : pattern_databases) {
        size += pdb->get_size();
    }

    cout << "Pruned " << num_cliques - max_cliques.size() <<
    " of " << num_cliques << " cliques" << endl;
    cout << "Pruned " << num_patterns - pattern_databases.size() <<
    " of " << num_patterns << " PDBs" << endl;

    cout << "Dominance pruning took " << timer << endl;
}

void CanonicalPDBsHeuristic::initialize() {
}

int CanonicalPDBsHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    return compute_heuristic(state);
}

int CanonicalPDBsHeuristic::compute_heuristic(const State &state) const {
    int max_h = 0;
    assert(!max_cliques.empty());
    // If we have an empty collection, then max_cliques = { \emptyset }.

    unordered_map<PatternDatabase *, int> pdb_h_values;
    pdb_h_values.reserve(pattern_databases.size());

    for (PatternDatabase *pdb : pattern_databases) {
        int h = pdb->get_value(state);
        if (h == numeric_limits<int>::max())
            return DEAD_END;
        pdb_h_values[pdb] = h;
    }

    for (const vector<PatternDatabase *> &clique : max_cliques) {
        int clique_h = 0;
        for (PatternDatabase *pdb : clique)
            clique_h += pdb_h_values[pdb];
        max_h = max(max_h, clique_h);
    }
    return max_h;
}

void CanonicalPDBsHeuristic::add_pattern(const vector<int> &pattern) {
    add_pdb_for_pattern(pattern);
    compute_max_cliques();
}


void CanonicalPDBsHeuristic::get_max_additive_subsets(
    const vector<int> &new_pattern,
    vector<vector<PatternDatabase *> > &max_additive_subsets) {
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

    for (const vector<PatternDatabase *> &clique : max_cliques) {
        // Take all patterns which are additive to new_pattern.
        vector<PatternDatabase *> subset;
        subset.reserve(clique.size());
        for (PatternDatabase *pdb : clique) {
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
        max_additive_subsets.push_back(vector<PatternDatabase *>());
    }
}

bool CanonicalPDBsHeuristic::is_dead_end(const State &state) const {
    for (PatternDatabase *pdb : pattern_databases)
        if (pdb->get_value(state) == numeric_limits<int>::max())
            return true;
    return false;
}

void CanonicalPDBsHeuristic::dump_cgraph(
    const vector<vector<int> > &cgraph) const {
    cout << "Compatibility graph" << endl;
    for (size_t i = 0; i < cgraph.size(); ++i) {
        cout << i << " adjacent to: " << cgraph[i] << endl;
    }
}

void CanonicalPDBsHeuristic::dump_cliques() const {
    assert(!max_cliques.empty());
    cout << max_cliques.size() << " maximal clique(s)" << endl;
    cout << "Maximal cliques are (";
    for (const vector<PatternDatabase *> &clique : max_cliques) {
        cout << "[";
        for (PatternDatabase *pdb : clique) {
            cout << "{ ";
            for (int var_id : pdb->get_pattern()) {
                cout << var_id << " ";
            }
            cout << "}";
        }
        cout << "]";
    }
    cout << ")" << endl;
}

void CanonicalPDBsHeuristic::dump() const {
    for (PatternDatabase *pdb : pattern_databases) {
        cout << pdb->get_pattern() << endl;
    }
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Canonical PDB",
        "The canonical pattern database heuristic is calculated as follows. "
        "For a given pattern collection C, the value of the "
        "canonical heuristic function is the maximum over all "
        "maximal additive subsets A in C, where the value for one subset "
        "S in A is the sum of the heuristic values for all patterns in S "
        "for a given state.");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");
    Heuristic::add_options_to_parser(parser);
    Options opts;
    parse_patterns(parser, opts);

    if (parser.dry_run())
        return 0;

    return new CanonicalPDBsHeuristic(opts);
}

static Plugin<Heuristic> _plugin("cpdbs", _parse);
