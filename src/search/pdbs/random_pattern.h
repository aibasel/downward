#ifndef PDBS_RANDOM_PATTERN_H
#define PDBS_RANDOM_PATTERN_H

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
// TODO: document
extern Pattern generate_random_pattern(
    int max_pdb_size,
    double max_time,
    utils::Verbosity verbosity,
    const std::shared_ptr<utils::RandomNumberGenerator> &rng,
    const TaskProxy &task_proxy,
    int goal_variable,
    std::vector<std::vector<int>> &cg_neighbors);

extern void add_random_pattern_bidirectional_option_to_parser(options::OptionParser &parser);
}

#endif
