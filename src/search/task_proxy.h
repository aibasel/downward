#ifndef TASK_PROXY_H
#define TASK_PROXY_H

#include "abstract_task.h"

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
class StateProxy;
class TaskProxy;
class VariableProxy;
class VariablesProxy;

// Currently, we still need to map Operators to GlobalOperators for some things
// like marking preferred operators. In the long run this mapping should go away.
class GlobalOperator;


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


class FactProxy {
    const AbstractTask &task;
    int var_id;
    int value;
public:
    FactProxy(const AbstractTask &task_, int var_id_, int value_);
    ~FactProxy() {}
    VariableProxy get_variable() const;
    int get_value() const {
        return value;
    }
};


class ConditionsProxy {
protected:
    const AbstractTask &task;
    explicit ConditionsProxy(const AbstractTask &task_)
        : task(task_) {}
public:
    using ItemType = FactProxy;
    virtual ~ConditionsProxy() {}
    virtual std::size_t size() const = 0;
    virtual FactProxy operator[](std::size_t index) const = 0;
};


class VariableProxy {
    const AbstractTask &task;
    int id;
public:
    VariableProxy(const AbstractTask &task_, int id_)
        : task(task_), id(id_) {}
    ~VariableProxy() {}
    int get_id() const {
        return id;
    }
    int get_domain_size() const {
        return task.get_variable_domain_size(id);
    }
    FactProxy get_fact(int index) const {
        assert(index < get_domain_size());
        return FactProxy(task, id, index);
    }
};


class VariablesProxy {
    const AbstractTask &task;
public:
    using ItemType = VariableProxy;
    explicit VariablesProxy(const AbstractTask &task_)
        : task(task_) {}
    ~VariablesProxy() {}
    std::size_t size() const {
        return task.get_num_variables();
    }
    VariableProxy operator[](std::size_t index) const {
        assert(index < size());
        return VariableProxy(task, index);
    }
};


class PreconditionsProxy : public ConditionsProxy {
    int op_index;
    bool is_axiom;
public:
    PreconditionsProxy(const AbstractTask &task_, int op_index_, bool is_axiom_)
        : ConditionsProxy(task_), op_index(op_index_), is_axiom(is_axiom_) {}
    ~PreconditionsProxy() {}
    std::size_t size() const override {
        return task.get_num_operator_preconditions(op_index, is_axiom);
    }
    FactProxy operator[](std::size_t fact_index) const override {
        assert(fact_index < size());
        std::pair<int, int> fact =
            task.get_operator_precondition(op_index, fact_index, is_axiom);
        return FactProxy(task, fact.first, fact.second);
    }
};


class EffectConditionsProxy : public ConditionsProxy {
    int op_index;
    int eff_index;
    bool is_axiom;
public:
    EffectConditionsProxy(
        const AbstractTask &task_, int op_index_, int eff_index_, bool is_axiom_)
        : ConditionsProxy(task_), op_index(op_index_), eff_index(eff_index_), is_axiom(is_axiom_) {}
    ~EffectConditionsProxy() {}
    std::size_t size() const override {
        return task.get_num_operator_effect_conditions(op_index, eff_index, is_axiom);
    }
    FactProxy operator[](std::size_t index) const override {
        assert(index < size());
        std::pair<int, int> fact =
            task.get_operator_effect_condition(op_index, eff_index, index, is_axiom);
        return FactProxy(task, fact.first, fact.second);
    }
};


class EffectProxy {
    const AbstractTask &task;
    int op_index;
    int eff_index;
    bool is_axiom;
public:
    EffectProxy(const AbstractTask &task_, int op_index_, int eff_index_, bool is_axiom_)
        : task(task_), op_index(op_index_), eff_index(eff_index_), is_axiom(is_axiom_) {}
    ~EffectProxy() {}
    EffectConditionsProxy get_conditions() const {
        return EffectConditionsProxy(task, op_index, eff_index, is_axiom);
    }
    FactProxy get_fact() const {
        std::pair<int, int> fact =
            task.get_operator_effect(op_index, eff_index, is_axiom);
        return FactProxy(task, fact.first, fact.second);
    }
};


class EffectsProxy {
    const AbstractTask &task;
    int op_index;
    bool is_axiom;
public:
    using ItemType = EffectProxy;
    EffectsProxy(const AbstractTask &task_, int op_index_, bool is_axiom_)
        : task(task_), op_index(op_index_), is_axiom(is_axiom_) {}
    ~EffectsProxy() {}
    std::size_t size() const {
        return task.get_num_operator_effects(op_index, is_axiom);
    }
    EffectProxy operator[](std::size_t eff_index) const {
        assert(eff_index < size());
        return EffectProxy(task, op_index, eff_index, is_axiom);
    }
};


class OperatorProxy {
    const AbstractTask &task;
    int index;
    bool is_an_axiom;
public:
    OperatorProxy(const AbstractTask &task_, int index_, bool is_axiom)
        : task(task_), index(index_), is_an_axiom(is_axiom) {}
    ~OperatorProxy() {}
    PreconditionsProxy get_preconditions() const {
        return PreconditionsProxy(task, index, is_an_axiom);
    }
    EffectsProxy get_effects() const {
        return EffectsProxy(task, index, is_an_axiom);
    }
    int get_cost() const {
        return task.get_operator_cost(index, is_an_axiom);
    }
    bool is_axiom() const {
        return is_an_axiom;
    }
    const std::string &get_name() const {
        return task.get_operator_name(index, is_an_axiom);
    }
    bool is_applicable(StateProxy &state) const;
    const GlobalOperator *get_global_operator() const {
        return task.get_global_operator(index, is_an_axiom);
    }
};


class OperatorsProxy {
    const AbstractTask &task;
public:
    using ItemType = OperatorProxy;
    explicit OperatorsProxy(const AbstractTask &task_)
        : task(task_) {}
    ~OperatorsProxy() {}
    std::size_t size() const {
        return task.get_num_operators();
    }
    OperatorProxy operator[](std::size_t index) const {
        assert(index < size());
        return OperatorProxy(task, index, false);
    }
};


class AxiomsProxy {
    const AbstractTask &task;
public:
    using ItemType = OperatorProxy;
    explicit AxiomsProxy(const AbstractTask &task_)
        : task(task_) {}
    ~AxiomsProxy() {}
    std::size_t size() const {
        return task.get_num_axioms();
    }
    OperatorProxy operator[](std::size_t index) const {
        assert(index < size());
        return OperatorProxy(task, index, true);
    }
};


class GoalsProxy : public ConditionsProxy {
public:
    explicit GoalsProxy(const AbstractTask &task_)
        : ConditionsProxy(task_) {}
    ~GoalsProxy() {}
    std::size_t size() const override {
        return task.get_num_goals();
    }
    FactProxy operator[](std::size_t index) const override {
        assert(index < size());
        std::pair<int, int> fact = task.get_goal_fact(index);
        return FactProxy(task, fact.first, fact.second);
    }
};


class StateProxy {
    const AbstractTask &task;
    const int index;
public:
    using ItemType = FactProxy;
    explicit StateProxy(const AbstractTask &task_, int index_)
        : task(task_), index(index_) {
    }
    ~StateProxy() {}
    std::size_t size() const {
        return task.get_num_variables();
    }
    FactProxy operator[](std::size_t var_id) const {
        assert(var_id < size());
        int value = task.get_variable_value_in_state(index, var_id);
        return FactProxy(task, var_id, value);
    }
};


class TaskProxy {
    const AbstractTask *task;
public:
    explicit TaskProxy(const AbstractTask *task_)
        : task(task_) {}
    ~TaskProxy() {
        delete task;
    }
    VariablesProxy get_variables() const {
        return VariablesProxy(*task);
    }
    OperatorsProxy get_operators() const {
        return OperatorsProxy(*task);
    }
    AxiomsProxy get_axioms() const {
        return AxiomsProxy(*task);
    }
    GoalsProxy get_goals() const {
        return GoalsProxy(*task);
    }
    StateProxy get_state(int state_id) const {
        return StateProxy(*task, state_id);
    }
};


inline FactProxy::FactProxy(const AbstractTask &task_, int var_id_, int value_)
    : task(task_), var_id(var_id_), value(value_) {
    assert(var_id >= 0 && var_id < task.get_num_variables());
    assert(value >= 0 && value < get_variable().get_domain_size());
}


inline VariableProxy FactProxy::get_variable() const {
    return VariableProxy(task, var_id);
}

inline bool OperatorProxy::is_applicable(StateProxy &state) const {
    for (FactProxy precondition : get_preconditions()) {
        std::size_t var_id = precondition.get_variable().get_id();
        if (precondition.get_value() != state[var_id].get_value())
            return false;
    }
    return true;
}

#endif
