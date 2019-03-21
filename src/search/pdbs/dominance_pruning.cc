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

      "pattern_cliques" is the vector of pattern cliques.

      The algorithm works by setting a "current pattern collection"
      against which other patterns and collections can be tested for
      dominance efficiently.

      "variable_to_pattern_id" encodes the relevant information about the
      current clique. For every variable v, variable_to_pattern_id[v] is
      the id of the pattern containing v in the current clique,
      or -1 if the variable is not contained in the current
      clique. (Note that patterns in a pattern clique must be
      disjoint, which is verified by an assertion in debug mode.)

      To test if a given pattern v_1, ..., v_k is dominated by the
      current clique, we check that all entries variable_to_pattern_id[v_i]
      are equal and different from -1.

      "dominated_patterns" is a vector<bool> that can be used to
      quickly test whether a given pattern is dominated by the current
      clique. This is precomputed for every pattern whenever the
      current clique is set.
    */

    const PatternCollection &patterns;
    const vector<PatternClique> &pattern_cliques;
    const int num_variables;

    vector<int> variable_to_pattern_id;
    vector<bool> dominated_patterns;

    void set_current_clique(int clique_id) {
        /*
          Set the current pattern collection to be used for
          is_pattern_dominated() or is_collection_dominated(). Compute
          dominated_patterns based on the current pattern collection.
        */
        variable_to_pattern_id.assign(num_variables, -1);
        assert(variable_to_pattern_id == vector<int>(num_variables, -1));
        for (PatternID pattern_id : pattern_cliques[clique_id]) {
            for (int variable : patterns[pattern_id]) {
                assert(variable_to_pattern_id[variable] == -1);
                variable_to_pattern_id[variable] = pattern_id;
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
          by the current pattern clique.
        */
        const Pattern &pattern = patterns[pattern_id];
        assert(!pattern.empty());
        PatternID clique_pattern_id = variable_to_pattern_id[pattern[0]];
        if (clique_pattern_id == -1) {
            return false;
        }
        int pattern_size = pattern.size();
        for (int i = 1; i < pattern_size; ++i) {
            if (variable_to_pattern_id[pattern[i]] != clique_pattern_id) {
                return false;
            }
        }
        return true;
    }

    bool is_clique_dominated(int clique_id) const {
        /*
          Check if the collection with the given collection_id is
          dominated by the current pattern collection.
        */
        for (PatternID pattern_id : pattern_cliques[clique_id]) {
            if (!dominated_patterns[pattern_id]) {
                return false;
            }
        }
        return true;
    }

public:
    Pruner(
        const PatternCollection &patterns,
        const vector<PatternClique> &pattern_cliques,
        int num_variables)
        : patterns(patterns),
          pattern_cliques(pattern_cliques),
          num_variables(num_variables) {
    }

    vector<bool> get_pruned_cliques(const utils::CountdownTimer &timer) {
        int num_cliques = pattern_cliques.size();
        vector<bool> pruned(num_cliques, false);
        /*
          Already pruned cliques are not used to prune other
          cliques. This makes things faster and helps handle
          duplicate cliques in the correct way: the first copy
          will survive and prune all duplicates.
        */
        for (int c1 = 0; c1 < num_cliques; ++c1) {
            if (!pruned[c1]) {
                set_current_clique(c1);
                for (int c2 = 0; c2 < num_cliques; ++c2) {
                    if (c1 != c2 && !pruned[c2] && is_clique_dominated(c2))
                        pruned[c2] = true;
                }
            }
            if (timer.is_expired()) {
                /*
                  Since after each iteration, we determined if a given
                  clique is pruned or not, we can just break the
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

void prune_dominated_cliques(
    PatternCollection &patterns,
    PDBCollection &pdbs,
    vector<PatternClique> &pattern_cliques,
    int num_variables,
    double max_time) {
    cout << "Running dominance pruning..." << endl;
    utils::CountdownTimer timer(max_time);

    int num_patterns = patterns.size();
    int num_cliques = pattern_cliques.size();

    vector<bool> pruned = Pruner(
        patterns,
        pattern_cliques,
        num_variables).get_pruned_cliques(timer);

    vector<PatternClique> remaining_pattern_cliques;
    vector<bool> is_remaining_pattern(num_patterns, false);
    int num_remaining_patterns = 0;
    for (size_t i = 0; i < pattern_cliques.size(); ++i) {
        if (!pruned[i]) {
            PatternClique &clique = pattern_cliques[i];
            for (PatternID pattern_id : clique) {
                if (!is_remaining_pattern[pattern_id]) {
                    is_remaining_pattern[pattern_id] = true;
                    ++num_remaining_patterns;
                }
            }
            remaining_pattern_cliques.push_back(move(clique));
        }
    }

    PatternCollection remaining_patterns;
    PDBCollection remaining_pdbs;
    remaining_patterns.reserve(num_remaining_patterns);
    remaining_pdbs.reserve(num_remaining_patterns);
    vector<PatternID> old_to_new_pattern_id(num_patterns, -1);
    for (PatternID old_pattern_id = 0; old_pattern_id < num_patterns; ++old_pattern_id) {
        if (is_remaining_pattern[old_pattern_id]) {
            PatternID new_pattern_id = remaining_patterns.size();
            old_to_new_pattern_id[old_pattern_id] = new_pattern_id;
            remaining_patterns.push_back(move(patterns[old_pattern_id]));
            remaining_pdbs.push_back(move(pdbs[old_pattern_id]));
        }
    }
    for (PatternClique &clique : remaining_pattern_cliques) {
        for (size_t i = 0; i < clique.size(); ++i) {
            PatternID old_pattern_id = clique[i];
            PatternID new_pattern_id = old_to_new_pattern_id[old_pattern_id];
            assert(new_pattern_id != -1);
            clique[i] = new_pattern_id;
        }
    }

    int num_pruned_collections = num_cliques - remaining_pattern_cliques.size();
    cout << "Pruned " << num_pruned_collections << " of " << num_cliques
         << " pattern cliques" << endl;

    int num_pruned_patterns = num_patterns - num_remaining_patterns;
    cout << "Pruned " << num_pruned_patterns << " of " << num_patterns
         << " PDBs" << endl;

    patterns.swap(remaining_patterns);
    pdbs.swap(remaining_pdbs);
    pattern_cliques.swap(remaining_pattern_cliques);

    cout << "Dominance pruning took " << timer.get_elapsed_time() << endl;
}
}
