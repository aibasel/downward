#ifndef TASK_H
#define TASK_H

#include "task_interface.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>


class AxiomsProxy;
class ConditionsProxy;
class EffectProxy;
class EffectConditionsProxy;
class EffectsProxy;
class FactProxy;
class GoalsProxy;
class OperatorProxy;
class OperatorsProxy;
class PreconditionsProxy;
class TaskProxy;
class VariableProxy;
class VariablesProxy;

// Currently, we still need to map Operators to GlobalOperators for some things
// like marking preferred operators. In the long run this mapping should go away.
class GlobalOperator;


class FactProxy {
    const AbstractTask &interface;
    int var_id;
    int value;
public:
    FactProxy(const AbstractTask &interface_, int var_id_, int value_);
    ~FactProxy() {}
    VariableProxy get_variable() const;
    int get_value() const {
        return value;
    }
};


class ConditionsProxy {
protected:
    const AbstractTask &interface;
    explicit ConditionsProxy(const AbstractTask &interface_)
        : interface(interface_) {}
public:
    using ItemType = FactProxy;
    virtual ~ConditionsProxy() {}
    virtual std::size_t size() const = 0;
    virtual FactProxy operator[](std::size_t index) const = 0;
};


class VariableProxy {
    const AbstractTask &interface;
    int id;
public:
    VariableProxy(const AbstractTask &interface_, int id_)
        : interface(interface_), id(id_) {}
    ~VariableProxy() {}
    int get_id() const {
        return id;
    }
    int get_domain_size() const {
        return interface.get_variable_domain_size(id);
    }
    FactProxy get_fact(int index) const {
        assert(index < get_domain_size());
        return FactProxy(interface, id, index);
    }
};


class VariablesProxy {
    const AbstractTask &interface;
public:
    using ItemType = VariableProxy;
    explicit VariablesProxy(const AbstractTask &interface_)
        : interface(interface_) {}
    ~VariablesProxy() {}
    std::size_t size() const {
        return interface.get_num_variables();
    }
    VariableProxy operator[](std::size_t index) const {
        assert(index < size());
        return VariableProxy(interface, index);
    }
};


class PreconditionsProxy : public ConditionsProxy {
    int op_index;
    bool is_axiom;
public:
    PreconditionsProxy(const AbstractTask &interface_, int op_index_, bool is_axiom_)
        : ConditionsProxy(interface_), op_index(op_index_), is_axiom(is_axiom_) {}
    ~PreconditionsProxy() {}
    std::size_t size() const override {
        return interface.get_num_operator_preconditions(op_index, is_axiom);
    }
    FactProxy operator[](std::size_t fact_index) const override {
        assert(fact_index < size());
        std::pair<int, int> fact =
            interface.get_operator_precondition(op_index, fact_index, is_axiom);
        return FactProxy(interface, fact.first, fact.second);
    }
};


class EffectConditionsProxy : public ConditionsProxy {
    int op_index;
    int eff_index;
    bool is_axiom;
public:
    EffectConditionsProxy(
        const AbstractTask &interface_, int op_index_, int eff_index_, bool is_axiom_)
        : ConditionsProxy(interface_), op_index(op_index_), eff_index(eff_index_), is_axiom(is_axiom_) {}
    ~EffectConditionsProxy() {}
    std::size_t size() const override {
        return interface.get_num_operator_effect_conditions(op_index, eff_index, is_axiom);
    }
    FactProxy operator[](std::size_t index) const override {
        assert(index < size());
        std::pair<int, int> fact =
            interface.get_operator_effect_condition(op_index, eff_index, index, is_axiom);
        return FactProxy(interface, fact.first, fact.second);
    }
};


class EffectProxy {
    const AbstractTask &interface;
    int op_index;
    int eff_index;
    bool is_axiom;
public:
    EffectProxy(const AbstractTask &interface_, int op_index_, int eff_index_, bool is_axiom_)
        : interface(interface_), op_index(op_index_), eff_index(eff_index_), is_axiom(is_axiom_) {}
    ~EffectProxy() {}
    EffectConditionsProxy get_conditions() const {
        return EffectConditionsProxy(interface, op_index, eff_index, is_axiom);
    }
    FactProxy get_fact() const {
        std::pair<int, int> fact =
            interface.get_operator_effect(op_index, eff_index, is_axiom);
        return FactProxy(interface, fact.first, fact.second);
    }
};


class EffectsProxy {
    const AbstractTask &interface;
    int op_index;
    bool is_axiom;
public:
    using ItemType = EffectProxy;
    EffectsProxy(const AbstractTask &interface_, int op_index_, bool is_axiom_)
        : interface(interface_), op_index(op_index_), is_axiom(is_axiom_) {}
    ~EffectsProxy() {}
    std::size_t size() const {
        return interface.get_num_operator_effects(op_index, is_axiom);
    }
    EffectProxy operator[](std::size_t eff_index) const {
        assert(eff_index < size());
        return EffectProxy(interface, op_index, eff_index, is_axiom);
    }
};


class OperatorProxy {
    const AbstractTask &interface;
    int index;
    bool is_an_axiom;
public:
    OperatorProxy(const AbstractTask &interface_, int index_, bool is_axiom)
        : interface(interface_), index(index_), is_an_axiom(is_axiom) {}
    ~OperatorProxy() {}
    PreconditionsProxy get_preconditions() const {
        return PreconditionsProxy(interface, index, is_an_axiom);
    }
    EffectsProxy get_effects() const {
        return EffectsProxy(interface, index, is_an_axiom);
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


class OperatorsProxy {
    const AbstractTask &interface;
public:
    using ItemType = OperatorProxy;
    explicit OperatorsProxy(const AbstractTask &interface_)
        : interface(interface_) {}
    ~OperatorsProxy() {}
    std::size_t size() const {
        return interface.get_num_operators();
    }
    OperatorProxy operator[](std::size_t index) const {
        assert(index < size());
        return OperatorProxy(interface, index, false);
    }
};


class AxiomsProxy {
    const AbstractTask &interface;
public:
    using ItemType = OperatorProxy;
    explicit AxiomsProxy(const AbstractTask &interface_)
        : interface(interface_) {}
    ~AxiomsProxy() {}
    std::size_t size() const {
        return interface.get_num_axioms();
    }
    OperatorProxy operator[](std::size_t index) const {
        assert(index < size());
        return OperatorProxy(interface, index, true);
    }
};


class GoalsProxy : public ConditionsProxy {
public:
    explicit GoalsProxy(const AbstractTask &interface_)
        : ConditionsProxy(interface_) {}
    ~GoalsProxy() {}
    std::size_t size() const override {
        return interface.get_num_goals();
    }
    FactProxy operator[](std::size_t index) const override {
        assert(index < size());
        std::pair<int, int> fact = interface.get_goal_fact(index);
        return FactProxy(interface, fact.first, fact.second);
    }
};


class TaskProxy {
    const AbstractTask *interface;
public:
    explicit TaskProxy(const AbstractTask *interface_)
        : interface(interface_) {}
    ~TaskProxy() {
        delete interface;
    }
    VariablesProxy get_variables() const {
        return VariablesProxy(*interface);
    }
    OperatorsProxy get_operators() const {
        return OperatorsProxy(*interface);
    }
    AxiomsProxy get_axioms() const {
        return AxiomsProxy(*interface);
    }
    GoalsProxy get_goals() const {
        return GoalsProxy(*interface);
    }
};


inline FactProxy::FactProxy(const AbstractTask &interface_, int var_id_, int value_)
    : interface(interface_), var_id(var_id_), value(value_) {
    assert(var_id >= 0 && var_id < interface.get_num_variables());
    assert(value >= 0 && value < get_variable().get_domain_size());
}


inline VariableProxy FactProxy::get_variable() const {
    return VariableProxy(interface, var_id);
}


// Basic iterator support for proxy classes.

template<class ProxyCollection>
class ProxyIterator {
    const ProxyCollection &collection;
    std::size_t pos;

public:
    ProxyIterator(const ProxyCollection &collection_, std::size_t pos_)
        : collection(collection_), pos(pos_) {}

    typename ProxyCollection::ItemType operator*() {
        return collection[pos];
    }
    ProxyIterator &operator++() {
        ++pos;
        return *this;
    }
    bool operator!=(const ProxyIterator &it) const {
        return pos != it.pos;
    }
};

template<class ProxyCollection>
inline ProxyIterator<ProxyCollection> begin(ProxyCollection &collection) {
    return ProxyIterator<ProxyCollection>(collection, 0);
}

template<class ProxyCollection>
inline ProxyIterator<ProxyCollection> end(ProxyCollection &collection) {
    return ProxyIterator<ProxyCollection>(collection, collection.size());
}

#endif
