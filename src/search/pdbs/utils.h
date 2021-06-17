#ifndef PDBS_UTILS_H
#define PDBS_UTILS_H

#include "types.h"

#include "../task_proxy.h"

#include "../utils/timer.h"

#include <memory>
#include <string>

namespace utils {
class RandomNumberGenerator;
}

namespace pdbs {
class PatternCollectionInformation;
class PatternInformation;

extern int compute_pdb_size(const TaskProxy &task_proxy, const Pattern &pattern);
extern int compute_total_pdb_size(
    const TaskProxy &task_proxy, const PatternCollection &pattern_collection);

extern std::vector<FactPair> get_goals_in_random_order(
    const TaskProxy &task_proxy, utils::RandomNumberGenerator &rng);
extern std::vector<int> get_non_goal_variables(const TaskProxy &task_proxy);
extern std::vector<std::vector<int>> compute_cg_neighbors(
    const std::shared_ptr<AbstractTask> &task,
    bool bidirectional);

/*
  Dump the given pattern, the number of variables contained, the size of the
  corresponding PDB, and the runtime used for computing it. All output is
  prepended with the given string identifier.
*/
extern void dump_pattern_generation_statistics(
    const std::string &identifier,
    utils::Duration runtime,
    const PatternInformation &pattern_info);

/*
  Compute and dump the number of patterns, the total size of the corresponding
  PDBs, and the runtime used for computing the collection. All output is
  prepended with the given string identifier.
*/
extern void dump_pattern_collection_generation_statistics(
    const std::string &identifier,
    utils::Duration runtime,
    const PatternCollectionInformation &pci);
}

#endif
