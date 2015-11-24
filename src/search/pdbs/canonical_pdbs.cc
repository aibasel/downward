#include "canonical_pdbs.h"

#include "dominance_pruner.h"
#include "pattern_database.h"

#include "../task_proxy.h"
#include "../timer.h"

#include <cassert>
#include <limits>

using namespace std;


CanonicalPDBs::CanonicalPDBs(shared_ptr<PDBCollection> pattern_databases,
                             shared_ptr<PDBCliques> max_cliques)
    : pattern_databases(pattern_databases),
      max_cliques(max_cliques) {
    assert(pattern_databases);
    assert(max_cliques);
}

int CanonicalPDBs::get_value(const State &state) const {
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

void CanonicalPDBs::dominance_pruning() {
    Timer timer;
    int num_patterns = pattern_databases->size();
    int num_cliques = max_cliques->size();

    DominancePruner(*pattern_databases, *max_cliques).prune();

    cout << "Pruned " << num_cliques - max_cliques->size() <<
        " of " << num_cliques << " cliques" << endl;
    cout << "Pruned " << num_patterns - pattern_databases->size() <<
        " of " << num_patterns << " PDBs" << endl;

    cout << "Dominance pruning took " << timer << endl;
}
