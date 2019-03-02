#ifndef PDBS_UTILS_H
#define PDBS_UTILS_H

#include "types.h"

#include "../utils/timer.h"

#include <memory>
#include <string>

class TaskProxy;

namespace pdbs {
extern int compute_pdb_size(const TaskProxy &task_proxy, const Pattern &pattern);
extern int compute_total_pdb_size(
    const TaskProxy &task_proxy, const PatternCollection &pattern_collection);
/*
   Dumps the following information for a given pattern collection or PDB
   collection, one of which has to be given as input:
   - the collection itself
   - the number of elements in the collection
   - the total PDB size of the collection
   - the runtime in seconds it took to compute the collection
   All output is prepended with the given string identifier.
*/
extern void dump_pattern_collection_statistics(
    const TaskProxy &task_proxy,
    std::string identifier,
    utils::Duration runtime,
    const std::shared_ptr<PatternCollection> &pattern_collection = nullptr,
    const std::shared_ptr<PDBCollection> &pdbs = nullptr);
}

#endif
