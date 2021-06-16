#ifndef PDBS_RCG_H
#define PDBS_RCG_H

#include "types.h"

#include <memory>

class TaskProxy;

namespace utils {
class RandomNumberGenerator;
}

namespace pdbs {
extern Pattern generate_pattern(
    int max_pdb_size,
    int goal_variable,
    const std::shared_ptr<utils::RandomNumberGenerator> &rng,
    const TaskProxy &task_proxy,
    std::vector<std::vector<int>> &cg_neighbors);
}


#endif
