#ifndef TASK_H
#define TASK_H

#include "globals.h"
#include "operator.h"

#include <cassert>
#include <cstddef>


class TaskImpl;
class OperatorRef;
class OperatorsRef;
class Task;


class TaskImpl {
public:
    virtual int get_operator_cost(std::size_t index) const = 0;
    virtual int get_adjusted_operator_cost(std::size_t index) const = 0;
    virtual size_t get_num_operators() const = 0;
};


class OperatorRef {
    const TaskImpl &impl;
    size_t index;
public:
    OperatorRef(const TaskImpl &impl_, std::size_t index_);
    ~OperatorRef();
    int get_cost() const {return impl.get_operator_cost(index); }
    int get_adjusted_cost() const {return impl.get_adjusted_operator_cost(index); }
    const Operator &get_original_operator() const {
        assert(index < g_operators.size());
        return g_operators[index];
    }
};


class OperatorsRef {
    const TaskImpl &impl;
public:
    OperatorsRef(const TaskImpl &impl_);
    ~OperatorsRef();
    std::size_t size() const {return impl.get_num_operators(); }
    const OperatorRef operator[](std::size_t index) const {return OperatorRef(impl, index); }
};


class Task {
    const TaskImpl &impl;
public:
    Task(const TaskImpl &impl_);
    ~Task();
    const OperatorsRef get_operators() const {return OperatorsRef(impl); }
};

#endif
