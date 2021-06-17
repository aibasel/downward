#ifndef PDBS_RCG_H
#define PDBS_RCG_H

#include "types.h"

#include <memory>

class TaskProxy;

namespace options {
class OptionParser;
}

namespace utils {
class RandomNumberGenerator;
enum class Verbosity;
}

namespace pdbs {
extern Pattern generate_pattern_rcg(
    int max_pdb_size,
    double max_time,
    int goal_variable,
    utils::Verbosity verbosity,
    const std::shared_ptr<utils::RandomNumberGenerator> &rng,
    const TaskProxy &task_proxy,
    std::vector<std::vector<int>> &cg_neighbors);

extern void add_rcg_options_to_parser(options::OptionParser &parser);
}

#endif
