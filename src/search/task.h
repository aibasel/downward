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
    virtual OperatorRef get_operator(std::size_t index) const = 0;
    virtual size_t get_number_of_operators() const = 0;
    virtual const OperatorsRef get_operators() const = 0;
};


class OperatorRef {
    const TaskImpl &task_impl;
    size_t index;
public:
    OperatorRef(const TaskImpl &task_impl_, std::size_t index_);
    ~OperatorRef();
    int get_cost() const {return task_impl.get_operator_cost(index); }
    const Operator &get_original_operator() const {
        assert(index < g_operators.size());
        return g_operators[index];
    }
};


class OperatorsRef {
    const TaskImpl &task_impl;
public:
    OperatorsRef(const TaskImpl &task_impl_);
    ~OperatorsRef();
    std::size_t size() const {return task_impl.get_number_of_operators(); }
    const OperatorRef operator[](std::size_t index) const {return task_impl.get_operator(index); }
};


class Task {
    const TaskImpl &task_impl;
public:
    Task(const TaskImpl &task_impl_);
    ~Task();
    const OperatorsRef get_operators() const {return task_impl.get_operators(); }
};

#endif
