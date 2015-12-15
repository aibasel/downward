#ifndef TASKS_DOMAIN_ABSTRACTED_TASK_FACTORY_H
#define TASKS_DOMAIN_ABSTRACTED_TASK_FACTORY_H

#include "domain_abstracted_task.h"

#include <memory>

class AbstractTask;

namespace Tasks {
std::shared_ptr<AbstractTask> build_domain_abstracted_task(
    const std::shared_ptr<AbstractTask> parent,
    const VarToGroups &value_groups);
}

#endif
