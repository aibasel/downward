#include "task.h"

using namespace std;

OperatorRef::OperatorRef(const TaskImpl &task_impl_, size_t index_)
    : task_impl(task_impl_),
      index(index_) {}

OperatorRef::~OperatorRef() {}

OperatorsRef::OperatorsRef(const TaskImpl &task_impl_)
    : task_impl(task_impl_) {}

OperatorsRef::~OperatorsRef() {}

Task::Task(const TaskImpl &task_impl_)
    : task_impl(task_impl_) {}

Task::~Task() {}
