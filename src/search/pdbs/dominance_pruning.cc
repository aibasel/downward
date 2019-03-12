#include "dominance_pruning.h"

#include "pattern_collection_information.h"
#include "pattern_database.h"

#include "../utils/countdown_timer.h"
#include "../utils/hash.h"

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
    const PatternCollection &patterns;
    const MaxAdditivePDBSubsets &collections;

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
    Pruner(PatternCollectionInformation &pci,
           int num_variables) :
        num_variables(num_variables),
        patterns(*pci.get_patterns()),
        collections(*pci.get_max_additive_subsets()) {
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

PatternCollectionInformation prune_dominated_subsets(
    PatternCollectionInformation &pci,
    int num_variables,
    double max_time) {
    cout << "Running dominance pruning..." << endl;
    utils::CountdownTimer timer(max_time);

    vector<bool> pruned = Pruner(
        pci,
        num_variables).get_pruned_collections(timer);

    const MaxAdditivePDBSubsets &max_additive_subsets =
        *pci.get_max_additive_subsets();
    shared_ptr<MaxAdditivePDBSubsets> remaining_max_additive_subsets =
        make_shared<MaxAdditivePDBSubsets>();
    unordered_set<int> remaining_pattern_indices;
    for (size_t i = 0; i < max_additive_subsets.size(); ++i) {
        if (!pruned[i]) {
            const vector<int> &subset = max_additive_subsets[i];
            remaining_max_additive_subsets->push_back(subset);
            for (int pattern_index : subset) {
                remaining_pattern_indices.insert(pattern_index);
            }
        }
    }

    /*
      TODO: we shouldn't call get_pdbs() here because they may not be computed
      and need not to be computed here. We currently do this because the only
      user of this class is the CPDB heuristic for which we know it computes
      PDBs anyways. Doing so allows us here to remove pruned patterns *and
      PDBs*.
    */
    const PatternCollection &patterns = *pci.get_patterns();
    const PDBCollection &pdbs = *pci.get_pdbs();
    shared_ptr<PatternCollection> remaining_patterns =
        make_shared<PatternCollection>();
    shared_ptr<PDBCollection> remaining_pdbs =
        make_shared<PDBCollection>();
    vector<int> old_to_new_pattern_index(patterns.size(), -1);
    for (int old_pattern_index : remaining_pattern_indices) {
        int new_pattern_index = remaining_patterns->size();
        old_to_new_pattern_index[old_pattern_index] = new_pattern_index;
        remaining_patterns->push_back(patterns[old_pattern_index]);
        remaining_pdbs->push_back(pdbs[old_pattern_index]);
    }
    for (vector<int> &subset : *remaining_max_additive_subsets) {
        for (size_t i = 0; i < subset.size(); ++i) {
            int old_pattern_index = subset[i];
            int new_pattern_index = old_to_new_pattern_index[old_pattern_index];
            assert(new_pattern_index != -1);
            subset[i] = new_pattern_index;
        }
    }

    int num_collections = max_additive_subsets.size();
    int num_pruned_collections = num_collections - remaining_max_additive_subsets->size();
    cout << "Pruned " << num_pruned_collections << " of " << num_collections
         << " maximal additive subsets" << endl;

    int num_patterns = patterns.size();
    int num_pruned_patterns = num_patterns - remaining_patterns->size();
    cout << "Pruned " << num_pruned_patterns << " of " << num_patterns
         << " PDBs" << endl;

    cout << "Dominance pruning took " << timer.get_elapsed_time() << endl;

    PatternCollectionInformation result(pci.get_task_proxy(), remaining_patterns);
    result.set_pdbs(remaining_pdbs);
    result.set_max_additive_subsets(remaining_max_additive_subsets);
    return result;
}
}
