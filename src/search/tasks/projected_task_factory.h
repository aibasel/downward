#ifndef TASKS_PROJECTED_TASK_FACTORY_H
#define TASKS_PROJECTED_TASK_FACTORY_H

#include <memory>
#include <vector>

class AbstractTask;

namespace extra_tasks {
extern std::shared_ptr<AbstractTask> build_projected_task(
    const std::shared_ptr<AbstractTask> &parent,
    std::vector<int> &&variables);
}

#endif
