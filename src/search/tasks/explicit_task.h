#ifndef TASKS_EXPLICIT_TASK_H
#define TASKS_EXPLICIT_TASK_H

#include "../abstract_task.h"

namespace tasks {
extern std::shared_ptr<AbstractTask> read_task(std::istream &in);
}
#endif
