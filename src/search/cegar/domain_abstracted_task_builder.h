#ifndef CEGAR_DOMAIN_ABSTRACTED_TASK_BUILDER_H
#define CEGAR_DOMAIN_ABSTRACTED_TASK_BUILDER_H

#include "domain_abstracted_task.h"

#include <memory>
#include <vector>

class AbstractTask;

namespace cegar {
std::shared_ptr<AbstractTask> build_domain_abstracted_task(
    const std::shared_ptr<AbstractTask> parent,
    const VarToGroups &value_groups);
}

#endif
