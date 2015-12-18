#ifndef TASKS_DOMAIN_ABSTRACTED_TASK_FACTORY_H
#define TASKS_DOMAIN_ABSTRACTED_TASK_FACTORY_H

#include "domain_abstracted_task.h"

#include <memory>

class AbstractTask;


namespace ExtraTasks {
/*
  Factory for creating domain abstractions.
*/
std::shared_ptr<DomainAbstractedTask> build_domain_abstracted_task(
    const std::shared_ptr<AbstractTask> parent,
    const VarToGroups &value_groups);
}

#endif
