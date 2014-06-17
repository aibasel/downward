#ifndef TASK_H
#define TASK_H

#include <cstddef>


class TaskFact;
class TaskVariable;
class TaskVariables;
class TaskCondition;
class TaskEffect;
class TaskEffects;
class TaskOperator;
class TaskOperators;
class TaskAxiom;
class TaskAxioms;
class TaskState;
class Task;


class TaskFact {
public:
    virtual const TaskVariable &get_variable() const = 0;
    virtual std::size_t get_value() const = 0;
};


class TaskVariable {
public:
    virtual std::size_t get_id() const = 0;
    virtual std::size_t get_domain_size() const = 0;
    virtual const TaskFact &get_fact(std::size_t index) const = 0;
};


class TaskVariables {
public:
    virtual std::size_t size() const = 0;
    virtual const TaskVariable &operator[](std::size_t index) const = 0;
};


class TaskCondition {
public:
    virtual std::size_t size() const = 0;
    virtual const TaskFact &operator[](std::size_t index) const = 0;
};


class TaskEffect {
public:
    virtual const TaskCondition &get_condition() const = 0;
    virtual const TaskFact &get_effect() const = 0;
};


class TaskEffects {
public:
    virtual std::size_t size() const = 0;
    virtual const TaskEffect &operator[](std::size_t index) const = 0;
};


class TaskOperator {
public:
    virtual const TaskCondition &get_precondition() const = 0;
    virtual const TaskEffects &get_effects() const = 0;
};


class TaskOperators {
public:
    virtual std::size_t size() const = 0;
    virtual const TaskOperator &operator[](std::size_t index) const = 0;
};


class TaskAxiom {
public:
    virtual const TaskCondition &get_condition() const = 0;
    virtual const TaskEffect &get_effect() const = 0;
};


class TaskAxioms {
public:
    virtual std::size_t size() const = 0;
    virtual const TaskAxiom &operator[](std::size_t index) const = 0;
};


class TaskState {
public:
    virtual std::size_t size() const = 0;
    virtual const TaskFact &operator[](std::size_t index) const = 0;
    virtual const TaskFact &operator[](const TaskVariable &var) const {
        return (*this)[var.get_id()];
    }
};


class Task {
public:
    virtual const TaskVariables &get_variables() const = 0;
    virtual const TaskOperators &get_operators() const = 0;
    virtual const TaskAxioms &get_axioms() const = 0;
    virtual const TaskState &get_initial_state() const = 0;
    virtual const TaskCondition &get_goal() const = 0;
};

#endif
