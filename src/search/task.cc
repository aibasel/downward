#include "task.h"

using namespace std;

Variable Fact::get_variable() const {
    return Variable(impl, var_id);
}

OperatorRef::OperatorRef(const TaskInterface &impl_, size_t index_)
    : impl(impl_),
      index(index_) {}

OperatorRef::~OperatorRef() {}

Operators::Operators(const TaskInterface &impl_)
    : impl(impl_) {}

Operators::~Operators() {}

Axioms::Axioms(const TaskInterface &impl_)
    : impl(impl_) {}

Axioms::~Axioms() {}

Task::Task(const TaskInterface &impl_)
    : impl(impl_) {}

Task::~Task() {}
