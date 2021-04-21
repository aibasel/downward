#ifndef PDBS_PATTERN_DATABASE_H
#define PDBS_PATTERN_DATABASE_H

#include "types.h"

#include <utility>
#include <vector>

class OperatorProxy;
class State;

namespace pdbs {
class PerfectHashFunction {
    Pattern pattern;
    // size of the PDB
    std::size_t num_states;
    // multipliers for each variable for perfect hash function
    std::vector<std::size_t> hash_multipliers;
public:
    PerfectHashFunction(
        Pattern &&pattern,
        std::size_t num_states,
        std::vector<std::size_t> &&hash_multipliers);
    /*
      The given concrete state is used to calculate the index of the
      according abstract state. This is only used for table lookup
      (distances) during search.
    */
    std::size_t rank(const std::vector<int> &state) const;
    int unrank(size_t index, int var, int domain_size) const;

    const Pattern &get_pattern() const {
        return pattern;
    }

    // Returns the size (number of abstract states) of the PDB
    std::size_t get_num_states() const {
        return num_states;
    }

    std::size_t get_multiplier(int var) const {
        return hash_multipliers[var];
    }
};

// Implements a single pattern database
class PatternDatabase {
    PerfectHashFunction hash_function;

    /*
      final h-values for abstract-states.
      dead-ends are represented by numeric_limits<int>::max()
    */
    std::vector<int> distances;
public:
    /*
      This is a one-shot constructor which should not be called by users
      wishing to create a PDB. Instead, they should use generate_pdb from file
      pattern_database_factory to create a PDB.
    */
    PatternDatabase(
        PerfectHashFunction &&hash_function,
        std::vector<int> &&distances);
    ~PatternDatabase() = default;

    // Return the PDB value of the given concrete state.
    int get_value(const std::vector<int> &state) const;

    // Returns the pattern (i.e. all variables used) of the PDB
    const Pattern &get_pattern() const {
        return hash_function.get_pattern();
    }

    // Returns the size (number of abstract states) of the PDB
    int get_size() const {
        return hash_function.get_num_states();
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
