#ifndef PDBS_UTIL_H
#define PDBS_UTIL_H

#include "types.h"

#include <vector>

class TaskProxy;

extern void validate_and_normalize_pattern(
    const TaskProxy &task_proxy, Pattern &pattern);
extern void validate_and_normalize_patterns(
    const TaskProxy &task_proxy, Patterns &patterns);


#endif
