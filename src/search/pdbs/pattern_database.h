#ifndef PDBS_PATTERN_DATABASE_H
#define PDBS_PATTERN_DATABASE_H

#include "types.h"

#include "../task_proxy.h"

#include <vector>

namespace pdbs {
class Projection {
    Pattern pattern;
    std::vector<int> domain_sizes;
    int num_abstract_states;
    std::vector<int> hash_multipliers;
public:
    Projection(const TaskProxy &task_proxy, const Pattern &pattern);

    // Compute the hash index (aka. the rank) of the given concrete state.
    int rank(const std::vector<int> &state) const;

    /*
      Compute the value of a given variable in the abstract state given as
      (hash) index.
    */
    int unrank(int index, int var) const;

    const Pattern &get_pattern() const {
        return pattern;
    }

    int get_num_abstract_states() const {
        return num_abstract_states;
    }

    int get_multiplier(int var) const {
        return hash_multipliers[var];
    }
};

class PatternDatabase {
    Projection projection;

    /*
      final h-values for abstract-states.
      dead-ends are represented by numeric_limits<int>::max()
    */
    std::vector<int> distances;
public:
    PatternDatabase(
        Projection &&projection,
        std::vector<int> &&distances);
    int get_value(const std::vector<int> &state) const;

    const Pattern &get_pattern() const {
        return projection.get_pattern();
    }

    // The size of the PDB is the number of abstract states.
    int get_size() const {
        return projection.get_num_abstract_states();
    }

    /*
      Return the average h-value over all states, where dead-ends are
      ignored (they neither increase the sum of all h-values nor the
      number of entries for the mean value calculation). If all states
      are dead-ends, return infinity.
      Note: This is only calculated when called; avoid repeated calls to
      this method!
    */
    double compute_mean_finite_h() const;
};
}

#endif
