#ifndef PDBS_STEEPEST_ASCENT_HILL_CLIMBING_H
#define PDBS_STEEPEST_ASCENT_HILL_CLIMBING_H

#include <memory>
#include <vector>

class OperatorID;
class TaskProxy;

namespace utils {
class RandomNumberGenerator;
enum class Verbosity;
}

namespace pdbs {
class PatternDatabase;

extern std::vector<std::vector<OperatorID>> steepest_ascent_enforced_hillclimbing(
    const TaskProxy &abs_task_proxy,
    const std::shared_ptr<utils::RandomNumberGenerator> &rng,
    const PatternDatabase &pdb,
    bool compute_wildcard_plan,
    utils::Verbosity verbosity);
}

#endif
