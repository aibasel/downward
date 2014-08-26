#ifndef TASK_H
#define TASK_H

#include "task_interface.h"

#include <cassert>
#include <cstddef>

class Fact;
class Facts;
class Variable;
class Variables;
class Preconditions;
class EffectConditions;
class Effect;
class Effects;
class OperatorRef;
class Operators;
class Axioms;
class Goals;
class Task;

class Operator;


class Fact {
    const TaskInterface &interface;
    std::size_t var_id;
    std::size_t value;
public:
    Fact(const TaskInterface &interface_, std::size_t var_id_, std::size_t value_)
        : interface(interface_), var_id(var_id_), value(value_) {}
    ~Fact() {}
    std::size_t get_var_id() const {return var_id; }
    std::size_t get_value() const {return value; }
};


class Facts {
protected:
    const TaskInterface &interface;
    explicit Facts(const TaskInterface &interface_) : interface(interface_) {}
public:
    ~Facts() {}
    virtual std::size_t size() const = 0;
    virtual Fact operator[](std::size_t index) const = 0;
};


class Variable {
    const TaskInterface &interface;
    std::size_t id;
public:
    Variable(const TaskInterface &interface_, std::size_t id_)
        : interface(interface_), id(id_) {}
    ~Variable() {}
    std::size_t get_id() const {return id; }
    std::size_t get_domain_size() const {return interface.get_variable_domain_size(id); }
    Fact get_fact(std::size_t index) const {return Fact(interface, id, index); }
};


class Variables {
    const TaskInterface &interface;
public:
    explicit Variables(const TaskInterface &interface_) : interface(interface_) {}
    ~Variables() {}
    std::size_t size() const {return interface.get_num_variables(); }
    Variable operator[](std::size_t index) const {return Variable(interface, index); }
};


class Preconditions : Facts {
    std::size_t op_index;
public:
    Preconditions(const TaskInterface &interface_, std::size_t op_index_)
        : Facts(interface_), op_index(op_index_) {}
    ~Preconditions() {}
    std::size_t size() const {return interface.get_num_operator_preconditions(op_index); }
    Fact operator[](std::size_t fact_index) const {
        std::pair<std::size_t, std::size_t> fact =
            interface.get_operator_precondition(op_index, fact_index);
        return Fact(interface, fact.first, fact.second);
    }
};


class EffectConditions : Facts {
    std::size_t op_index;
    std::size_t eff_index;
public:
    EffectConditions(
        const TaskInterface &interface_, std::size_t op_index_, std::size_t eff_index_)
        : Facts(interface_), op_index(op_index_), eff_index(eff_index_) {}
    ~EffectConditions() {}
    std::size_t size() const {
        return interface.get_num_operator_effect_conditions(op_index, eff_index);
    }
    Fact operator[](std::size_t index) const {
        std::pair<std::size_t, std::size_t> fact =
            interface.get_operator_effect_condition(op_index, eff_index, index);
        return Fact(interface, fact.first, fact.second);
    }
};


class Effect {
    const TaskInterface &interface;
    std::size_t op_index;
    std::size_t eff_index;
public:
    Effect(const TaskInterface &interface_, std::size_t op_index_, std::size_t eff_index_)
        : interface(interface_), op_index(op_index_), eff_index(eff_index_) {}
    ~Effect() {}
    EffectConditions get_conditions() const {
        return EffectConditions(interface, op_index, eff_index);
    }
    Fact get_effect() const {
        std::pair<std::size_t, std::size_t> fact =
            interface.get_operator_effect(op_index, eff_index);
        return Fact(interface, fact.first, fact.second);
    }
};


class Effects {
    const TaskInterface &interface;
    std::size_t op_index;
public:
    Effects(const TaskInterface &interface_, std::size_t op_index_)
        : interface(interface_), op_index(op_index_) {}
    ~Effects() {}
    std::size_t size() const {return interface.get_num_operator_effects(op_index); }
    Effect operator[](std::size_t eff_index) const {
        return Effect(interface, op_index, eff_index);
    }
};


class OperatorRef {
    const TaskInterface &interface;
    size_t index;
    bool is_an_axiom;
public:
    OperatorRef(const TaskInterface &interface_, std::size_t index_, bool is_axiom)
        : interface(interface_), index(index_), is_an_axiom(is_axiom) {}
    ~OperatorRef() {}
    Preconditions get_preconditions() const {return Preconditions(interface, index); }
    Effects get_effects() const {return Effects(interface, index); }
    int get_cost() const {return interface.get_operator_cost(index, is_an_axiom); }
    bool is_axiom() const {return is_an_axiom; }
    const Operator *get_original_operator() const {
        return interface.get_original_operator(index);
    }
};


class Operators {
    const TaskInterface &interface;
public:
    explicit Operators(const TaskInterface &interface_) : interface(interface_) {}
    ~Operators() {}
    std::size_t size() const {return interface.get_num_operators(); }
    OperatorRef operator[](std::size_t index) const {
        return OperatorRef(interface, index, false);
    }
};


class Axioms {
    const TaskInterface &interface;
public:
    explicit Axioms(const TaskInterface &interface_) : interface(interface_) {}
    ~Axioms() {}
    std::size_t size() const {return interface.get_num_axioms(); }
    OperatorRef operator[](std::size_t index) const {
        return OperatorRef(interface, index, true);
    }
};


class Goals : Facts {
public:
    explicit Goals(const TaskInterface &interface_) : Facts(interface_) {}
    ~Goals() {}
    std::size_t size() const {return interface.get_num_goals(); }
    Fact operator[](std::size_t index) const {
        std::pair<std::size_t, std::size_t> fact = interface.get_goal_fact(index);
        return Fact(interface, fact.first, fact.second);
    }
};


class Task {
    const TaskInterface &interface;
public:
    explicit Task(const TaskInterface &interface_) : interface(interface_) {}
    ~Task() {}
    Variables get_variables() const {return Variables(interface); }
    Operators get_operators() const {return Operators(interface); }
    Axioms get_axioms() const {return Axioms(interface); }
    Goals get_goals() const {return Goals(interface); }
};

#endif
