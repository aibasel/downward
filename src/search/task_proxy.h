#ifndef TASK_PROXY_H
#define TASK_PROXY_H

#include "abstract_task.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>


class AxiomsProxy;
class ConditionsProxy;
class EffectProxy;
class EffectConditionsProxy;
class EffectsProxy;
class FactProxy;
class FactsProxy;
class GoalsProxy;
class OperatorProxy;
class OperatorsProxy;
class PreconditionsProxy;
class State;
class TaskProxy;
class VariableProxy;
class VariablesProxy;

/*
  Overview of the task interface.

  The task interface is divided into two parts: a set of proxy classes
  for accessing task information (TaskProxy, OperatorProxy, etc.) and
  task implementations (subclasses of AbstractTask). Each proxy class
  knows which AbstractTask it belongs to and uses its methods to retrieve
  information about the task. RootTask is the AbstractTask that
  encapsulates the unmodified original task that the planner received
  as input.

  Example code for creating a new task object and accessing its operators:

      TaskProxy task = new TaskProxy(new RootTask());
      for (OperatorProxy op : task->get_operators())
          cout << op.get_name() << endl;

  Since proxy classes only store a reference to the AbstractTask and some
  indices, they can be copied cheaply and be passed by value instead of
  by reference.

  In addition to the lightweight proxy classes, the task interface
  consists of the State class, which is used to hold state information
  for TaskProxy tasks. The State class provides methods similar to the
  proxy classes, but since State objects own the state data they should
  be passed by reference.

  For now, only the heuristics work with the TaskProxy classes and hence
  potentially on a transformed view of the original task. The search
  algorithms keep working on the original unmodified task using the
  GlobalState, GlobalOperator etc. classes. We therefore need to do two
  conversions: converting GlobalStates to State objects for the heuristic
  computation and converting OperatorProxy objects used by the heuristic
  to GlobalOperators for reporting preferred operators. These conversions
  are done by the Heuristic base class. Until all heuristics use the new
  task interface, heuristics can use Heuristic::convert_global_state() to
  convert GlobalStates to States. Afterwards, the heuristics are passed a
  State object directly. To mark operators as preferred, heuristics can
  use Heuristic::set_preferred() which currently works for both
  OperatorProxy and GlobalOperator objects.

      int FantasyHeuristic::compute_heuristic(const GlobalState &global_state) {
          State state = convert_global_state(global_state);
          set_preferred(task->get_operators()[42]);
          int sum = 0;
          for (FactProxy fact : state)
              sum += fact.get_value();
          return sum;
      }

  For helper functions that work on task related objects, please see the
  task_tools.h module.
*/


// Basic iterator support for proxy classes.

template<class ProxyCollection>
class ProxyIterator {
    const ProxyCollection &collection;
    std::size_t pos;
public:
    ProxyIterator(const ProxyCollection &collection, std::size_t pos)
        : collection(collection), pos(pos) {}

    typename ProxyCollection::ItemType operator*() const {
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
    FactProxy(const AbstractTask &task, int var_id, int value);
    ~FactProxy() {}
    VariableProxy get_variable() const;
    int get_value() const {
        return value;
    }
    const std::string &get_name() const {
        return task.get_fact_name(var_id, value);
    }
    bool operator==(FactProxy const &other) const {
        return var_id == other.var_id && value == other.value;
    }
    bool operator!=(FactProxy other) const {
        return !(*this == other);
    }
};


class FactsProxyIterator {
    const AbstractTask &task;
    int var_id;
    int value;
public:
    FactsProxyIterator(const AbstractTask &task, int var_id, int value)
        : task(task), var_id(var_id), value(value) {}
    ~FactsProxyIterator() {}
    FactProxy operator*() const {
        return FactProxy(task, var_id, value);
    }
    FactsProxyIterator &operator++() {
        assert(var_id < task.get_num_variables());
        int num_facts = task.get_variable_domain_size(var_id);
        assert(value < num_facts);
        ++value;
        if (value == num_facts) {
            ++var_id;
            value = 0;
        }
        return *this;
    }
    bool operator==(const FactsProxyIterator &other) const {
        return var_id == other.var_id && value == other.value;
    }
    bool operator!=(const FactsProxyIterator &other) const {
        return !(*this == other);
    }
};


/*
  Proxy class for the collection of all facts of a task.

  We don't implement size() because it would not be constant-time.
*/
class FactsProxy {
    const AbstractTask &task;
public:
    explicit FactsProxy(const AbstractTask &task)
        : task(task) {}
    ~FactsProxy() {}
    FactsProxyIterator begin() const {
        return FactsProxyIterator(task, 0, 0);
    }
    FactsProxyIterator end() const {
        return FactsProxyIterator(task, task.get_num_variables(), 0);
    }
};


class ConditionsProxy {
protected:
    const AbstractTask &task;
    explicit ConditionsProxy(const AbstractTask &task)
        : task(task) {}
public:
    using ItemType = FactProxy;
    virtual ~ConditionsProxy() {}
    virtual std::size_t size() const = 0;
    virtual FactProxy operator[](std::size_t index) const = 0;
    bool empty() const {
        return size() == 0;
    }
};


class VariableProxy {
    const AbstractTask &task;
    int id;
public:
    VariableProxy(const AbstractTask &task, int id)
        : task(task), id(id) {}
    ~VariableProxy() {}
    int get_id() const {
        return id;
    }
    const std::string &get_name() const {
        return task.get_variable_name(id);
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
    explicit VariablesProxy(const AbstractTask &task)
        : task(task) {}
    ~VariablesProxy() {}
    std::size_t size() const {
        return task.get_num_variables();
    }
    VariableProxy operator[](std::size_t index) const {
        assert(index < size());
        return VariableProxy(task, index);
    }
    FactsProxy get_facts() const {
        return FactsProxy(task);
    }
};


class PreconditionsProxy : public ConditionsProxy {
    int op_index;
    bool is_axiom;
public:
    PreconditionsProxy(const AbstractTask &task, int op_index, bool is_axiom)
        : ConditionsProxy(task), op_index(op_index), is_axiom(is_axiom) {}
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
        const AbstractTask &task, int op_index, int eff_index, bool is_axiom)
        : ConditionsProxy(task), op_index(op_index), eff_index(eff_index), is_axiom(is_axiom) {}
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
    EffectProxy(const AbstractTask &task, int op_index, int eff_index, bool is_axiom)
        : task(task), op_index(op_index), eff_index(eff_index), is_axiom(is_axiom) {}
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
    EffectsProxy(const AbstractTask &task, int op_index, bool is_axiom)
        : task(task), op_index(op_index), is_axiom(is_axiom) {}
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
    OperatorProxy(const AbstractTask &task, int index, bool is_axiom)
        : task(task), index(index), is_an_axiom(is_axiom) {}
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
    int get_id() const {
        return index;
    }
    const GlobalOperator *get_global_operator() const {
        return task.get_global_operator(index, is_an_axiom);
    }
};


class OperatorsProxy {
    const AbstractTask &task;
public:
    using ItemType = OperatorProxy;
    explicit OperatorsProxy(const AbstractTask &task)
        : task(task) {}
    ~OperatorsProxy() {}
    std::size_t size() const {
        return task.get_num_operators();
    }
    bool empty() const {
        return size() == 0;
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
    explicit AxiomsProxy(const AbstractTask &task)
        : task(task) {}
    ~AxiomsProxy() {}
    std::size_t size() const {
        return task.get_num_axioms();
    }
    bool empty() const {
        return size() == 0;
    }
    OperatorProxy operator[](std::size_t index) const {
        assert(index < size());
        return OperatorProxy(task, index, true);
    }
};


class GoalsProxy : public ConditionsProxy {
public:
    explicit GoalsProxy(const AbstractTask &task)
        : ConditionsProxy(task) {}
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


class State {
    const AbstractTask *task;
    const std::vector<int> values;
public:
    using ItemType = FactProxy;
    State(const AbstractTask &task, std::vector<int> && values)
        : task(&task), values(values) {
        assert(static_cast<int>(size()) == this->task->get_num_variables());
    }
    ~State() {}
    State(State && other)
        : task(other.task), values(std::move(other.values)) {
        other.task = 0;
    }
    std::size_t size() const {
        return values.size();
    }
    FactProxy operator[](std::size_t var_id) const {
        assert(var_id < size());
        return FactProxy(*task, var_id, values[var_id]);
    }
    FactProxy operator[](VariableProxy var) const {
        return (*this)[var.get_id()];
    }
};


class TaskProxy {
    const AbstractTask *task;
public:
    explicit TaskProxy(const AbstractTask *task)
        : task(task) {}
    ~TaskProxy() {
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
    State get_initial_state() const {
        return State(*task, task->get_initial_state_values());
    }
    State convert_global_state(const GlobalState &global_state) const {
        return State(*task, task->get_state_values(global_state));
    }
};


inline FactProxy::FactProxy(const AbstractTask &task, int var_id, int value)
    : task(task), var_id(var_id), value(value) {
    assert(var_id >= 0 && var_id < task.get_num_variables());
    assert(value >= 0 && value < get_variable().get_domain_size());
}


inline VariableProxy FactProxy::get_variable() const {
    return VariableProxy(task, var_id);
}

#endif
