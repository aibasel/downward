#ifndef PDBS_VALIDATION_H
#define PDBS_VALIDATION_H

#include "types.h"

class TaskProxy;

namespace pdbs {
extern void validate_and_normalize_pattern(
    const TaskProxy &task_proxy, Pattern &pattern);
extern void validate_and_normalize_patterns(
    const TaskProxy &task_proxy, PatternCollection &patterns);
}

#endif
