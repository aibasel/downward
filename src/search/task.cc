#include "task.h"

OperatorRef::OperatorRef(const TaskImpl &task_impl_, size_t index_)
    : task_impl(task_impl_),
      index(index_) {}

OperatorsRef::OperatorsRef(const TaskImpl &task_impl_)
    : task_impl(task_impl_) {}

Task::Task(const TaskImpl &task_impl_)
    : task_impl(task_impl_) {}
