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
    const Pattern &pattern);

/*
  Dump the given pattern collection, the number of patterns, the total size
  of the corresponding PDBs, and the runtime used for computing the collection.
  All output is prepended with the given string identifier.
*/
extern void dump_pattern_collection_generation_statistics(
    const std::string &identifier,
    utils::Duration runtime,
    const PatternCollectionInformation &pci);
}

#endif
