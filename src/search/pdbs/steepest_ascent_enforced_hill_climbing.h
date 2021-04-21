#ifndef PDBS_STEEPEST_ASCENT_ENFORCED_HILL_CLIMBING_H
#define PDBS_STEEPEST_ASCENT_ENFORCED_HILL_CLIMBING_H

#include "types.h"

#include <memory>
#include <vector>

class OperatorID;

namespace utils {
class RandomNumberGenerator;
enum class Verbosity;
}

namespace pdbs {
class AbstractOperators;
class PerfectHashFunction;
class Projection;

extern std::vector<std::vector<OperatorID>> steepest_ascent_enforced_hill_climbing(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const std::vector<int> &distances,
    const AbstractOperators &abstract_operators,
    const std::shared_ptr<utils::RandomNumberGenerator> &rng,
    bool compute_wildcard_plan,
    utils::Verbosity verbosity,
    std::size_t initial_state);
}

#endif
