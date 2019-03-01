#ifndef PDBS_UTILS_H
#define PDBS_UTILS_H

#include "types.h"

class TaskProxy;

namespace pdbs {
extern int compute_pdb_size(const TaskProxy &task_proxy, const Pattern &pattern);
extern int compute_total_pdb_size(
    const TaskProxy &task_proxy, const PatternCollection &pattern_collection);
}

#endif
