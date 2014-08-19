#include "task.h"

using namespace std;

OperatorRef::OperatorRef(const TaskInterface &impl_, size_t index_)
    : impl(impl_),
      index(index_) {}

OperatorRef::~OperatorRef() {}

OperatorsRef::OperatorsRef(const TaskInterface &impl_)
    : impl(impl_) {}

OperatorsRef::~OperatorsRef() {}

Axioms::Axioms(const TaskInterface &impl_)
    : impl(impl_) {}

Axioms::~Axioms() {}

Task::Task(const TaskInterface &impl_)
    : impl(impl_) {}

Task::~Task() {}
