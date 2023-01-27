#ifndef PDBS_RANDOM_PATTERN_H
#define PDBS_RANDOM_PATTERN_H

#include "types.h"

#include <memory>

class TaskProxy;

namespace plugins {
class Feature;
}

namespace utils {
class LogProxy;
class RandomNumberGenerator;
}

namespace pdbs {
/*
  This function computes a pattern for the given task. Starting with the given
  goal variable, the algorithm executes a random walk on the causal graph. In
  each iteration, it selects a random causal graph neighbor of the current
  variable (given via cg_neighbors). It terminates if no neighbor fits the
  pattern due to the size limit or if the time limit is reached.
*/
extern Pattern generate_random_pattern(
    int max_pdb_size,
    double max_time,
    utils::LogProxy &log,
    const std::shared_ptr<utils::RandomNumberGenerator> &rng,
    const TaskProxy &task_proxy,
    int goal_variable,
    std::vector<std::vector<int>> &cg_neighbors);

extern void add_random_pattern_implementation_notes_to_feature(
    plugins::Feature &feature);
extern void add_random_pattern_bidirectional_option_to_feature(
    plugins::Feature &feature);
}

#endif
