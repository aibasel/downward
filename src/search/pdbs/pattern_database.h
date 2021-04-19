#ifndef PDBS_PATTERN_DATABASE_H
#define PDBS_PATTERN_DATABASE_H

#include "types.h"

#include "../task_proxy.h"

#include <utility>
#include <vector>

namespace pdbs {
// Implements a single pattern database
class PatternDatabase {
    Pattern pattern;

    // size of the PDB
    std::size_t num_states;

    /*
      final h-values for abstract-states.
      dead-ends are represented by numeric_limits<int>::max()
    */
    std::vector<int> distances;

    // multipliers for each variable for perfect hash function
    std::vector<std::size_t> hash_multipliers;

    /*
      The given concrete state is used to calculate the index of the
      according abstract state. This is only used for table lookup
      (distances) during search.
    */
    std::size_t hash_index_of_concrete_state(const std::vector<int> &state) const;
public:
    /*
      This is a one-shot constructor which should not be called by users
      wishing to create a PDB. Instead, they should use PatternDatabaseFactory
      for creating a PDB.
    */
    PatternDatabase(
        const Pattern &pattern,
        std::size_t num_states,
        std::vector<int> &&distances,
        std::vector<std::size_t> &&hash_multipliers);
    ~PatternDatabase() = default;

    /*
      Like hash_index_of_concrete_state, compute the hash index of the
      given state. However, the given state must be a State of a ProjectedTask
      using pattern for the projected variables.
    */
    std::size_t hash_index_of_projected_state(const State &projected_state) const;

    // Return the PDB value of the given concrete state.
    int get_value(const std::vector<int> &state) const;

    // Return the PDB value of a state with the given hash index.
    int get_value_for_hash_index(std::size_t index) const;

    // Returns the pattern (i.e. all variables used) of the PDB
    const Pattern &get_pattern() const {
        return pattern;
    }

    // Returns the size (number of abstract states) of the PDB
    int get_size() const {
        return num_states;
    }

    /*
      Returns the average h-value over all states, where dead-ends are
      ignored (they neither increase the sum of all h-values nor the
      number of entries for the mean value calculation). If all states
      are dead-ends, infinity is returned.
      Note: This is only calculated when called; avoid repeated calls to
      this method!
    */
    double compute_mean_finite_h() const;

    // Returns true iff op has an effect on a variable in the pattern.
    bool is_operator_relevant(const OperatorProxy &op) const;
};
}

#endif
