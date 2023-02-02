#ifndef PDBS_PATTERN_DATABASE_H
#define PDBS_PATTERN_DATABASE_H

#include "types.h"

#include "../task_proxy.h"

#include <vector>

namespace pdbs {
class PatternDatabase {
    Pattern pattern;

    // size of the PDB
    int num_states;

    /*
      final h-values for abstract-states.
      dead-ends are represented by numeric_limits<int>::max()
    */
    std::vector<int> distances;

    // multipliers for each variable for perfect hash function
    std::vector<int> hash_multipliers;

    /*
      The given concrete state is used to calculate the index of the
      according abstract state. This is only used for table lookup
      (distances) during search.
    */
    int hash_index(const std::vector<int> &state) const;
public:
    PatternDatabase(
        Pattern &&pattern,
        int num_states,
        std::vector<int> &&distances,
        std::vector<int> &&hash_multipliers);
    ~PatternDatabase() = default;

    int get_value(const std::vector<int> &state) const;

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
};
}

#endif
