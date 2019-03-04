#ifndef PDBS_UTILS_H
#define PDBS_UTILS_H

#include "types.h"

#include "../utils/timer.h"

#include <memory>
#include <string>

class TaskProxy;

namespace pdbs {
class PatternCollectionInformation;

extern int compute_pdb_size(const TaskProxy &task_proxy, const Pattern &pattern);
extern int compute_total_pdb_size(
    const TaskProxy &task_proxy, const PatternCollection &pattern_collection);
/*
  Dump the given pattern, the number of variables contained, the size of the
  corresponding PDB, and the runtime used for computing it. All output is
  prepended with the given string identifier.
*/
extern void dump_pattern_generation_statistics(
    const TaskProxy &task_proxy,
    const std::string &identifier,
    utils::Duration runtime,
    const Pattern &pattern,
    const std::shared_ptr<PatternDatabase> &pdb = nullptr);
/*
  Dump the following information for a given pattern collection information:
  - the pattern collection itself
  - the number of elements in the collection
  - the total PDB size of the collection
  - the runtime in seconds it took to compute the collection
  All output is prepended with the given string identifier.
*/
extern void dump_pattern_collection_generation_statistics(
    const TaskProxy &task_proxy,
    const std::string &identifier,
    utils::Duration runtime,
    PatternCollectionInformation &pci);
}

#endif
