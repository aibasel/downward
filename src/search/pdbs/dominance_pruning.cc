#include "dominance_pruning.h"

#include "pattern_database.h"

#include "../utils/hash.h"
#include "../utils/timer.h"

#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

namespace pdbs {
class Pruner {
    /*
      Algorithm for pruning dominated patterns.

      "patterns" is the vector of patterns used.
      Each pattern is a vector of variable IDs.
      This is logically const (only changed in the constructor).

      "collections" is the vector of pattern collections.
      Each collection is a vector<int>, where each int is an index
      into "patterns".
      This is logically const (only changed in the constructor).

      The algorithm works by setting a "current pattern collection"
      against which other patterns and collections can be tested for
      dominance efficiently.

      "pattern_index" encodes the relevant information about the
      current collection. For every variable v, pattern_index[v] is
      the index of the pattern containing v in the current collection,
      or -1 if the variable is not contained in the current
      collection. (Note that patterns in a collection must be
      disjoint, which is verified by an assertion in debug mode.)

      To test if a given pattern v_1, ..., v_k is dominated by the
      current collection, we check that all entries pattern_index[v_i]
      are equal and different from -1.

      "dominated_patterns" is a vector<bool> that can be used to
      quickly test whether a given pattern is dominated by the current
      collection. This is precomputed for every pattern whenever the
      current collection is set.
    */

    const int num_variables;
    vector<Pattern> patterns;
    vector<vector<int>> collections;

    vector<int> pattern_index;
    vector<bool> dominated_patterns;

    void set_current_collection(int collection_id) {
        /*
          Set the current pattern collection to be used for
          is_pattern_dominated() or is_collection_dominated(). Compute
          dominated_patterns based on the current pattern collection.
        */
        pattern_index.assign(num_variables, -1);
        assert(pattern_index == vector<int>(num_variables, -1));
        for (int pattern_id : collections[collection_id]) {
            for (int variable : patterns[pattern_id]) {
                assert(pattern_index[variable] == -1);
                pattern_index[variable] = pattern_id;
            }
        }

        dominated_patterns.clear();
        dominated_patterns.reserve(patterns.size());
        for (size_t i = 0; i < patterns.size(); ++i)
            dominated_patterns.push_back(is_pattern_dominated(i));
    }

    bool is_pattern_dominated(int pattern_id) const {
        /*
          Check if the pattern with the given pattern_id is dominated
          by the current pattern collection.
        */
        const vector<int> &pattern = patterns[pattern_id];
        assert(!pattern.empty());
        int collection_pattern = pattern_index[pattern[0]];
        if (collection_pattern == -1)
            return false;
        int pattern_size = pattern.size();
        for (int i = 1; i < pattern_size; ++i)
            if (pattern_index[pattern[i]] != collection_pattern)
                return false;
        return true;
    }

    bool is_collection_dominated(int collection_id) const {
        /*
          Check if the collection with the given collection_id is
          dominated by the current pattern collection.
        */
        for (int pattern_id : collections[collection_id])
            if (!dominated_patterns[pattern_id])
                return false;
        return true;
    }

public:
    Pruner(const PDBCollection &pattern_databases,
           const MaxAdditivePDBSubsets &max_additive_subsets,
           int num_variables) :
        num_variables(num_variables) {
        unordered_map<const PatternDatabase *, int> pdb_to_pattern_index;

        int num_patterns = pattern_databases.size();
        patterns.reserve(num_patterns);
        for (int i = 0; i < num_patterns; ++i) {
            const PatternDatabase *pdb = pattern_databases[i].get();
            patterns.push_back(pdb->get_pattern());
            pdb_to_pattern_index[pdb] = i;
        }

        int num_collections = max_additive_subsets.size();
        collections.reserve(num_collections);
        for (const PDBCollection &collection : max_additive_subsets) {
            vector<int> pattern_indices;
            pattern_indices.reserve(collection.size());
            for (const shared_ptr<PatternDatabase> &pdb : collection) {
                assert(pdb_to_pattern_index.count(pdb.get()));
                pattern_indices.push_back(pdb_to_pattern_index[pdb.get()]);
            }
            collections.push_back(move(pattern_indices));
        }
    }

    vector<bool> get_pruned_collections() {
        int num_collections = collections.size();
        vector<bool> pruned(num_collections, false);
        /*
          Already pruned collections are not used to prune other
          collections. This makes things faster and helps handle
          duplicate collections in the correct way: the first copy
          will survive and prune all duplicates.
        */
        for (int c1 = 0; c1 < num_collections; ++c1) {
            if (!pruned[c1]) {
                set_current_collection(c1);
                for (int c2 = 0; c2 < num_collections; ++c2) {
                    if (c1 != c2 && !pruned[c2] && is_collection_dominated(c2))
                        pruned[c2] = true;
                }
            }
        }

        return pruned;
    }
};

shared_ptr<MaxAdditivePDBSubsets> prune_dominated_subsets(
    const PDBCollection &pattern_databases,
    const MaxAdditivePDBSubsets &max_additive_subsets,
    int num_variables) {
    utils::Timer timer;

    vector<bool> pruned = Pruner(
        pattern_databases,
        max_additive_subsets,
        num_variables).get_pruned_collections();

    shared_ptr<MaxAdditivePDBSubsets> result =
        make_shared<MaxAdditivePDBSubsets>();
    for (size_t i = 0; i < max_additive_subsets.size(); ++i)
        if (!pruned[i])
            result->push_back(max_additive_subsets[i]);

    int num_collections = max_additive_subsets.size();
    int num_pruned_collections = num_collections - result->size();
    cout << "Pruned " << num_pruned_collections << " of " << num_collections
         << " maximal additive subsets" << endl;

    unordered_set<PatternDatabase *> remaining_pdbs;
    for (const PDBCollection &collection : *result) {
        for (const shared_ptr<PatternDatabase> &pdb : collection) {
            remaining_pdbs.insert(pdb.get());
        }
    }
    int num_patterns = pattern_databases.size();
    int num_pruned_patterns = num_patterns - remaining_pdbs.size();
    cout << "Pruned " << num_pruned_patterns << " of " << num_patterns
         << " PDBs" << endl;

    cout << "Dominance pruning took " << timer << endl;

    return result;
}
}
