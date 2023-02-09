#ifndef PDBS_PATTERN_DATABASE_FACTORY_H
#define PDBS_PATTERN_DATABASE_FACTORY_H

#include "types.h"

#include "../task_proxy.h"

#include <memory>
#include <tuple>
#include <vector>

namespace utils {
class RandomNumberGenerator;
}

namespace pdbs {
/*
  Compute a PDB for the given task and pattern.

  The given pattern must be sorted, contain no duplicates and be small enough
  so that the number of abstract states is below numeric_limits<int>::max().
  If operator_costs is given, it must contain one integer for each operator
  of the task, specifying the cost that should be considered for that operator
  instead of its original cost.
*/
extern std::shared_ptr<PatternDatabase> compute_pdb(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    const std::vector<int> &operator_costs = std::vector<int>(),
    const std::shared_ptr<utils::RandomNumberGenerator> &rng = nullptr);

/*
  In addition to computing a PDB for the given task and pattern like
  compute_pdb() above, also compute an abstract plan along.

  A wildcard plan is a sequence of set of operator IDs, where each such set
  induces the same transition between the two abstract states of this plan
  step. That is, a wildcard plan induces a single trace of abstract states
  like a regular plan, but offers a set of "equivalent" operators for each
  plan step.

  If compute_wildcard_plan is false, each set of operator IDs of the returned
  plan contains exactly one operator, thus representing a regular plan. If
  set to true, each set contains at least one operator ID.
*/
extern std::tuple<std::shared_ptr<PatternDatabase>,
                  std::vector<std::vector<OperatorID>>> compute_pdb_and_plan(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    const std::vector<int> &operator_costs = std::vector<int>(),
    const std::shared_ptr<utils::RandomNumberGenerator> &rng = nullptr,
    bool compute_wildcard_plan = false);
}

#endif
