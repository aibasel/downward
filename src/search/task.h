#ifndef TASK_H
#define TASK_H

#include "globals.h"
#include "operator.h"
#include "operator_cost.h"

#include <cassert>
#include <cstddef>

class Fact;
class Variable;
class Condition;
class Precondition;
class TaskInterface;
class OperatorRef;
class Operators;
class Axioms;
class Task;


class TaskInterface {
public:
    virtual std::size_t get_num_variables() const = 0;
    virtual std::size_t get_variable_domain_size(std::size_t var) const = 0;

    virtual int get_operator_cost(std::size_t index) const = 0;
    virtual int get_adjusted_operator_cost(
        std::size_t index, OperatorCost cost_type) const = 0;
    virtual std::size_t get_num_operators() const = 0;
    virtual std::size_t get_operator_precondition_size(std::size_t index) const = 0;
    virtual std::pair<std::size_t, std::size_t> get_operator_precondition_fact(
        std::size_t op_index, std::size_t fact_index) const = 0;
    virtual std::size_t get_num_operator_effects(std::size_t op_index) const = 0;
    virtual std::size_t get_operator_effect_condition_size(
        std::size_t op_index, std::size_t eff_index) const = 0;
    virtual std::pair<std::size_t, std::size_t> get_operator_effect_condition(
        std::size_t op_index, std::size_t eff_index, std::size_t cond_index) const = 0;
    virtual std::pair<std::size_t, std::size_t> get_operator_effect(
        std::size_t op_index, std::size_t eff_index) const = 0;
    virtual const Operator *get_original_operator(std::size_t index) const = 0;

    virtual std::size_t get_num_axioms() const = 0;

    virtual std::size_t get_goal_size() const = 0;
    virtual std::pair<std::size_t, std::size_t> get_goal_fact(std::size_t index) const = 0;
};


class Fact {
    const TaskInterface &impl;
    std::size_t var_id;
    std::size_t value;
public:
    Fact(const TaskInterface &impl_, std::size_t var_id_, std::size_t value_)
        : impl(impl_), var_id(var_id_), value(value_) {}
    ~Fact() {}
    std::size_t get_value() const {return value; }
    Variable get_variable() const;
};


class Variable {
    const TaskInterface &impl;
    std::size_t id;
public:
    Variable(const TaskInterface &impl_, std::size_t id_) : impl(impl_), id(id_) {}
    ~Variable() {}
    std::size_t get_id() const {return id; }
    std::size_t get_domain_size() const {return impl.get_variable_domain_size(id); }
    Fact get_fact(std::size_t index) const {return Fact(impl, id, index); }
};


class Variables {
    const TaskInterface &impl;
public:
    Variables(const TaskInterface &impl_) : impl(impl_) {}
    ~Variables() {}
    std::size_t size() const {return impl.get_num_variables(); }
    Variable operator[](std::size_t index) const {return Variable(impl, index); }
};


class Condition {
protected:
    const TaskInterface &impl;
public:
    Condition(const TaskInterface &impl_) : impl(impl_) {}
    ~Condition() {}
    virtual std::size_t size() const = 0;
    virtual Fact operator[](std::size_t index) const = 0;
};


class Precondition : Condition {
    std::size_t op_index;
public:
    Precondition(const TaskInterface &impl_, std::size_t op_index_)
        : Condition(impl_), op_index(op_index_) {}
    ~Precondition() {}
    std::size_t size() const {return impl.get_operator_precondition_size(op_index); }
    Fact operator[](std::size_t fact_index) const {
        std::pair<std::size_t, std::size_t> fact =
            impl.get_operator_precondition_fact(op_index, fact_index);
        return Fact(impl, fact.first, fact.second);
    }
};


class Goal : Condition {
public:
    Goal(const TaskInterface &impl_) : Condition(impl_) {}
    ~Goal() {}
    std::size_t size() const {return impl.get_goal_size(); }
    Fact operator[](std::size_t index) const {
        std::pair<std::size_t, std::size_t> fact = impl.get_goal_fact(index);
        return Fact(impl, fact.first, fact.second);
    }
};


class EffectCondition : Condition {
    std::size_t op_index;
    std::size_t eff_index;
public:
    EffectCondition(
        const TaskInterface &impl_, std::size_t op_index_, std::size_t eff_index_)
        : Condition(impl_), op_index(op_index_), eff_index(eff_index_) {}
    ~EffectCondition() {}
    std::size_t size() const {
        return impl.get_operator_effect_condition_size(op_index, eff_index);
    }
    Fact operator[](std::size_t index) const {
        std::pair<std::size_t, std::size_t> fact =
            impl.get_operator_effect_condition(op_index, eff_index, index);
        return Fact(impl, fact.first, fact.second);
    }
};


class Effect {
    const TaskInterface &impl;
    std::size_t op_index;
    std::size_t eff_index;
public:
    Effect(const TaskInterface &impl_, std::size_t op_index_, std::size_t eff_index_)
        : impl(impl_), op_index(op_index_), eff_index(eff_index_) {}
    ~Effect() {}
    EffectCondition get_condition() const {
        return EffectCondition(impl, op_index, eff_index);
    }
    Fact get_effect() const {
        std::pair<std::size_t, std::size_t> fact =
            impl.get_operator_effect(op_index, eff_index);
        return Fact(impl, fact.first, fact.second);
    }
};


class Effects {
    const TaskInterface &impl;
    std::size_t op_index;
public:
    Effects(const TaskInterface &impl_, std::size_t op_index_)
        : impl(impl_), op_index(op_index_) {}
    ~Effects() {}
    std::size_t size() const {return impl.get_num_operator_effects(op_index); }
    Effect operator[](std::size_t eff_index) const {
        return Effect(impl, op_index, eff_index);
    }
};


class OperatorRef {
    const TaskInterface &impl;
    size_t index;
public:
    OperatorRef(const TaskInterface &impl_, std::size_t index_)
        : impl(impl_), index(index_) {}
    ~OperatorRef() {}
    Precondition get_precondition() const {return Precondition(impl, index); }
    Effects get_effects() const {return Effects(impl, index); }
    int get_cost() const {return impl.get_operator_cost(index); }
    int get_adjusted_cost(OperatorCost cost_type) const {
        return impl.get_adjusted_operator_cost(index, cost_type);
    }
};


class Operators {
    const TaskInterface &impl;
public:
    Operators(const TaskInterface &impl_) : impl(impl_) {}
    ~Operators() {}
    std::size_t size() const {return impl.get_num_operators(); }
    OperatorRef operator[](std::size_t index) const {
        return OperatorRef(impl, index);
    }
};


class Axioms {
    const TaskInterface &impl;
public:
    Axioms(const TaskInterface &impl_) : impl(impl_) {}
    ~Axioms() {}
    std::size_t size() const {return impl.get_num_axioms(); }
    OperatorRef operator[](std::size_t index) const {
        return OperatorRef(impl, index);
    }
};


class Task {
    const TaskInterface &impl;
public:
    Task(const TaskInterface &impl_) : impl(impl_) {}
    ~Task() {}
    Variables get_variables() const {return Variables(impl); }
    Operators get_operators() const {return Operators(impl); }
    const Operator *get_original_operator(std::size_t index) const {
        return impl.get_original_operator(index);
    }
    Axioms get_axioms() const {return Axioms(impl); }
    Goal get_goal() const {return Goal(impl); }
};

#endif
