#ifndef TASK_H
#define TASK_H

#include "globals.h"
#include "operator.h"
#include "operator_cost.h"

#include <cassert>
#include <cstddef>

class TaskInterface;
class OperatorRef;
class OperatorsRef;
class Task;


class TaskInterface {
public:
    virtual int get_operator_cost(std::size_t index) const = 0;
    virtual int get_adjusted_operator_cost(std::size_t index, OperatorCost cost_type) const = 0;
    virtual size_t get_num_operators() const = 0;
    virtual size_t get_num_axioms() const = 0;
};


class OperatorRef {
    const TaskInterface &impl;
    size_t index;
public:
    OperatorRef(const TaskInterface &impl_, std::size_t index_);
    ~OperatorRef();
    int get_cost() const {return impl.get_operator_cost(index); }
    int get_adjusted_cost(OperatorCost cost_type) const {
        return impl.get_adjusted_operator_cost(index, cost_type);
    }
    const Operator &get_original_operator() const {
        assert(index < g_operators.size());
        return g_operators[index];
    }
};


class OperatorsRef {
    const TaskInterface &impl;
public:
    OperatorsRef(const TaskInterface &impl_);
    ~OperatorsRef();
    std::size_t size() const {return impl.get_num_operators(); }
    OperatorRef operator[](std::size_t index) const {return OperatorRef(impl, index); }
};


class Axioms {
    const TaskInterface &impl;
public:
    Axioms(const TaskInterface &impl_);
    ~Axioms();
    std::size_t size() const {return impl.get_num_axioms(); }
    OperatorRef operator[](std::size_t index) const {return OperatorRef(impl, index); }
};


class Task {
    const TaskInterface &impl;
public:
    Task(const TaskInterface &impl_);
    ~Task();
    OperatorsRef get_operators() const {return OperatorsRef(impl); }
    Axioms get_axioms() const {return Axioms(impl); }
};

#endif
