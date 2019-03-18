#include "dominance_pruning.h"

#include "pattern_database.h"

#include "../utils/countdown_timer.h"
#include "../utils/hash.h"

#include <cassert>
#include <unordered_map>
#include <vector>

using namespace std;

namespace pdbs {
class Pruner {
    /*
      Algorithm for pruning dominated patterns.

      "patterns" is the vector of patterns used.
      Each pattern is a vector of variable IDs.

      "collections" is the vector of pattern collections.
      Each collection is a vector<int>, where each int is an index
      into "patterns".

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

    const PatternCollection &patterns;
    const MaxAdditivePDBSubsets &collections;
    const int num_variables;

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
        for (size_t i = 0; i < patterns.size(); ++i) {
            dominated_patterns.push_back(is_pattern_dominated(i));
        }
    }

    bool is_pattern_dominated(int pattern_id) const {
        /*
          Check if the pattern with the given pattern_id is dominated
          by the current pattern collection.
        */
        const vector<int> &pattern = patterns[pattern_id];
        assert(!pattern.empty());
        int collection_pattern = pattern_index[pattern[0]];
        if (collection_pattern == -1) {
            return false;
        }
        int pattern_size = pattern.size();
        for (int i = 1; i < pattern_size; ++i) {
            if (pattern_index[pattern[i]] != collection_pattern) {
                return false;
            }
        }
        return true;
    }

    bool is_collection_dominated(int collection_id) const {
        /*
          Check if the collection with the given collection_id is
          dominated by the current pattern collection.
        */
        for (int pattern_id : collections[collection_id]) {
            if (!dominated_patterns[pattern_id]) {
                return false;
            }
        }
        return true;
    }

public:
    Pruner(
        const PatternCollection &patterns,
        const MaxAdditivePDBSubsets &max_additive_subsets,
        int num_variables)
        : patterns(patterns),
          collections(max_additive_subsets),
          num_variables(num_variables) {
    }

    vector<bool> get_pruned_collections(const utils::CountdownTimer &timer) {
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
            if (timer.is_expired()) {
                /*
                  Since after each iteration, we determined if a given
                  collection is pruned or not, we can just break the
                  computation here if reaching the time limit and use all
                  information we collected so far.
                */
                cout << "Time limit reached. Abort dominance pruning." << endl;
                break;
            }
        }

        return pruned;
    }
};

void prune_dominated_subsets(
    PatternCollection &patterns,
    PDBCollection &pdbs,
    MaxAdditivePDBSubsets &max_additive_subsets,
    int num_variables,
    double max_time) {
    cout << "Running dominance pruning..." << endl;
    utils::CountdownTimer timer(max_time);

    int num_patterns = patterns.size();
    int num_subsets = max_additive_subsets.size();

    vector<bool> pruned = Pruner(
        patterns,
        max_additive_subsets,
        num_variables).get_pruned_collections(timer);

    MaxAdditivePDBSubsets remaining_max_additive_subsets;
    vector<bool> is_remaining_pattern(num_patterns, false);
    for (size_t i = 0; i < max_additive_subsets.size(); ++i) {
        if (!pruned[i]) {
            vector<int> &subset = max_additive_subsets[i];
            for (int pattern_index : subset) {
                is_remaining_pattern[pattern_index] = true;
            }
            remaining_max_additive_subsets.push_back(move(subset));
        }
    }

    int num_remaining_patterns = 0;
    for (int old_pattern_index = 0; old_pattern_index < num_patterns; ++old_pattern_index) {
        if (is_remaining_pattern[old_pattern_index]) {
            ++num_remaining_patterns;
        }
    }

    PatternCollection remaining_patterns;
    PDBCollection remaining_pdbs;
    remaining_patterns.reserve(num_remaining_patterns);
    remaining_pdbs.reserve(num_remaining_patterns);
    vector<int> old_to_new_pattern_index(num_patterns, -1);
    for (int old_pattern_index = 0; old_pattern_index < num_patterns; ++old_pattern_index) {
        if (is_remaining_pattern[old_pattern_index]) {
            int new_pattern_index = remaining_patterns.size();
            old_to_new_pattern_index[old_pattern_index] = new_pattern_index;
            remaining_patterns.push_back(move(patterns[old_pattern_index]));
            remaining_pdbs.push_back(move(pdbs[old_pattern_index]));
        }
    }
    for (vector<int> &subset : remaining_max_additive_subsets) {
        for (size_t i = 0; i < subset.size(); ++i) {
            int old_pattern_index = subset[i];
            int new_pattern_index = old_to_new_pattern_index[old_pattern_index];
            assert(new_pattern_index != -1);
            subset[i] = new_pattern_index;
        }
    }

    int num_pruned_collections = num_subsets - remaining_max_additive_subsets.size();
    cout << "Pruned " << num_pruned_collections << " of " << num_subsets
         << " maximal additive subsets" << endl;

    int num_pruned_patterns = num_patterns - num_remaining_patterns;
    cout << "Pruned " << num_pruned_patterns << " of " << num_patterns
         << " PDBs" << endl;

    patterns.swap(remaining_patterns);
    pdbs.swap(remaining_pdbs);
    max_additive_subsets.swap(remaining_max_additive_subsets);

    cout << "Dominance pruning took " << timer.get_elapsed_time() << endl;
}
}
