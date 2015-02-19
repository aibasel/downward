#ifndef TASK_H
#define TASK_H

#include "task_interface.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>


class Axioms;
class Conditions;
class Effect;
class EffectConditions;
class Effects;
class Fact;
class Goals;
class Operator;
class Operators;
class Preconditions;
class Task;
class Variable;
class Variables;

// Currently, we still need to map Operators to GlobalOperators for some things
// like marking preferred operators. In the long run this mapping should go away.
class GlobalOperator;


class Fact {
    const TaskInterface &interface;
    int var_id;
    int value;
public:
    Fact(const TaskInterface &interface_, int var_id_, int value_);
    ~Fact() {}
    Variable get_variable() const;
    int get_value() const {
        return value;
    }
};


class Conditions {
protected:
    const TaskInterface &interface;
    explicit Conditions(const TaskInterface &interface_)
        : interface(interface_) {}
public:
    virtual ~Conditions() {}
    virtual std::size_t size() const = 0;
    virtual Fact operator[](std::size_t index) const = 0;
};


class Variable {
    const TaskInterface &interface;
    int id;
public:
    Variable(const TaskInterface &interface_, int id_)
        : interface(interface_), id(id_) {}
    ~Variable() {}
    int get_id() const {
        return id;
    }
    int get_domain_size() const {
        return interface.get_variable_domain_size(id);
    }
    Fact get_fact(int index) const {
        assert(index < get_domain_size());
        return Fact(interface, id, index);
    }
};


class Variables {
    const TaskInterface &interface;
public:
    explicit Variables(const TaskInterface &interface_)
        : interface(interface_) {}
    ~Variables() {}
    std::size_t size() const {
        return interface.get_num_variables();
    }
    Variable operator[](std::size_t index) const {
        assert(index < size());
        return Variable(interface, index);
    }
};


class Preconditions : public Conditions {
    int op_index;
    bool is_axiom;
public:
    Preconditions(const TaskInterface &interface_, int op_index_, bool is_axiom_)
        : Conditions(interface_), op_index(op_index_), is_axiom(is_axiom_) {}
    ~Preconditions() {}
    std::size_t size() const override {
        return interface.get_num_operator_preconditions(op_index, is_axiom);
    }
    Fact operator[](std::size_t fact_index) const override {
        assert(fact_index < size());
        std::pair<int, int> fact =
            interface.get_operator_precondition(op_index, fact_index, is_axiom);
        return Fact(interface, fact.first, fact.second);
    }
};


class EffectConditions : public Conditions {
    int op_index;
    int eff_index;
    bool is_axiom;
public:
    EffectConditions(
        const TaskInterface &interface_, int op_index_, int eff_index_, bool is_axiom_)
        : Conditions(interface_), op_index(op_index_), eff_index(eff_index_), is_axiom(is_axiom_) {}
    ~EffectConditions() {}
    std::size_t size() const override {
        return interface.get_num_operator_effect_conditions(op_index, eff_index, is_axiom);
    }
    Fact operator[](std::size_t index) const override {
        assert(index < size());
        std::pair<int, int> fact =
            interface.get_operator_effect_condition(op_index, eff_index, index, is_axiom);
        return Fact(interface, fact.first, fact.second);
    }
};


class Effect {
    const TaskInterface &interface;
    int op_index;
    int eff_index;
    bool is_axiom;
public:
    Effect(const TaskInterface &interface_, int op_index_, int eff_index_, bool is_axiom_)
        : interface(interface_), op_index(op_index_), eff_index(eff_index_), is_axiom(is_axiom_) {}
    ~Effect() {}
    EffectConditions get_conditions() const {
        return EffectConditions(interface, op_index, eff_index, is_axiom);
    }
    Fact get_fact() const {
        std::pair<int, int> fact =
            interface.get_operator_effect(op_index, eff_index, is_axiom);
        return Fact(interface, fact.first, fact.second);
    }
};


class Effects {
    const TaskInterface &interface;
    int op_index;
    bool is_axiom;
public:
    Effects(const TaskInterface &interface_, int op_index_, bool is_axiom_)
        : interface(interface_), op_index(op_index_), is_axiom(is_axiom_) {}
    ~Effects() {}
    std::size_t size() const {
        return interface.get_num_operator_effects(op_index, is_axiom);
    }
    Effect operator[](std::size_t eff_index) const {
        assert(eff_index < size());
        return Effect(interface, op_index, eff_index, is_axiom);
    }
};


class Operator {
    const TaskInterface &interface;
    int index;
    bool is_an_axiom;
public:
    Operator(const TaskInterface &interface_, int index_, bool is_axiom)
        : interface(interface_), index(index_), is_an_axiom(is_axiom) {}
    ~Operator() {}
    Preconditions get_preconditions() const {
        return Preconditions(interface, index, is_an_axiom);
    }
    Effects get_effects() const {
        return Effects(interface, index, is_an_axiom);
    }
    int get_cost() const {
        return interface.get_operator_cost(index, is_an_axiom);
    }
    bool is_axiom() const {
        return is_an_axiom;
    }
    const std::string &get_name() const {
        return interface.get_operator_name(index, is_an_axiom);
    }
    const GlobalOperator *get_global_operator() const {
        return interface.get_global_operator(index, is_an_axiom);
    }
};


class Operators {
    const TaskInterface &interface;
public:
    explicit Operators(const TaskInterface &interface_)
        : interface(interface_) {}
    ~Operators() {}
    std::size_t size() const {
        return interface.get_num_operators();
    }
    Operator operator[](std::size_t index) const {
        assert(index < size());
        return Operator(interface, index, false);
    }
};


class Axioms {
    const TaskInterface &interface;
public:
    explicit Axioms(const TaskInterface &interface_)
        : interface(interface_) {}
    ~Axioms() {}
    std::size_t size() const {
        return interface.get_num_axioms();
    }
    Operator operator[](std::size_t index) const {
        assert(index < size());
        return Operator(interface, index, true);
    }
};


class Goals : public Conditions {
public:
    explicit Goals(const TaskInterface &interface_)
        : Conditions(interface_) {}
    ~Goals() {}
    std::size_t size() const override {
        return interface.get_num_goals();
    }
    Fact operator[](std::size_t index) const override {
        assert(index < size());
        std::pair<int, int> fact = interface.get_goal_fact(index);
        return Fact(interface, fact.first, fact.second);
    }
};


template<class Collection, class Item>
class Iterator {
    Collection collection;
    std::size_t pos;

public:
    Iterator(Collection collection_, std::size_t pos_)
        : collection(collection_), pos(pos_) {}

    Item operator*() {
        return collection[pos];
    }
    Iterator& operator++() {
        ++pos;
        return *this;
    }
    bool operator!=(const Iterator& it) const {
        return pos != it.pos;
    }
};

template<class Collection>
inline Iterator<Collection, Fact> begin(Collection& collection) {
   return {collection, 0};
}

template<class Collection>
inline Iterator<Collection, Fact> end(Collection& collection) {
   return {collection, collection.size()};
}


class Task {
    const TaskInterface *interface;
public:
    explicit Task(const TaskInterface *interface_)
        : interface(interface_) {}
    ~Task() {
        delete interface;
    }
    Variables get_variables() const {
        return Variables(*interface);
    }
    Operators get_operators() const {
        return Operators(*interface);
    }
    Axioms get_axioms() const {
        return Axioms(*interface);
    }
    Goals get_goals() const {
        return Goals(*interface);
    }
};


inline Fact::Fact(const TaskInterface &interface_, int var_id_, int value_)
    : interface(interface_), var_id(var_id_), value(value_) {
    assert(var_id >= 0 && var_id < interface.get_num_variables());
    assert(value >= 0 && value < get_variable().get_domain_size());
}


inline Variable Fact::get_variable() const {
    return Variable(interface, var_id);
}


#endif
