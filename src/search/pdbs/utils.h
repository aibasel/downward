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
    const TaskProxy &task_proxy, const std::shared_ptr<utils::RandomNumberGenerator> &rng);
extern std::vector<int> get_non_goal_variables(const TaskProxy &task_proxy);

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
  Dump the given pattern collection (unless dump_collection = false), the
  number of patterns, the total size of the corresponding PDBs, and the
  runtime used for computing the collection. If collection_size is different
  from -1, this is assumed to be the total size of all PDBs. Otherwise, this
  will be computed for the pattern collection. All output is prepended with
  the given string identifier.
*/
extern void dump_pattern_collection_generation_statistics(
    const std::string &identifier,
    utils::Duration runtime,
    const PatternCollectionInformation &pci,
    int collection_size = -1,
    bool dump_collection = true);
}

#endif
