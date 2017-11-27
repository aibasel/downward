#ifndef TASK_PROXY_H
#define TASK_PROXY_H

#include "abstract_task.h"
#include "global_state.h"
#include "operator_id.h"

#include "utils/collections.h"
#include "utils/hash.h"
#include "utils/system.h"

#include <cassert>
#include <cstddef>
#include <iterator>
#include <string>
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

namespace causal_graph {
class CausalGraph;
}

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

      TaskProxy task_proxy(*g_root_task());
      for (OperatorProxy op : task->get_operators())
          cout << op.get_name() << endl;

  Since proxy classes only store a reference to the AbstractTask and
  some indices, they can be copied cheaply.

  In addition to the lightweight proxy classes, the task interface
  consists of the State class, which is used to hold state information
  for TaskProxy tasks. The State class provides methods similar to the
  proxy classes, but since State objects own the state data they should
  be passed by reference.

  For now, only the heuristics work with the TaskProxy classes and
  hence potentially on a transformed view of the original task. The
  search algorithms keep working on the original unmodified task using
  the GlobalState, GlobalOperator etc. classes. We therefore need to do
  two conversions between the search and the heuristics: converting
  GlobalStates to State objects for the heuristic computation and
  converting OperatorProxy objects used by the heuristic to
  GlobalOperators for reporting preferred operators. These conversions
  are done by the Heuristic base class. Until all heuristics use the
  new task interface, heuristics can use
  Heuristic::convert_global_state() to convert GlobalStates to States.
  Afterwards, the heuristics are passed a State object directly. To
  mark operators as preferred, heuristics can use
  Heuristic::set_preferred() which currently works for both
  OperatorProxy and GlobalOperator objects.

      int FantasyHeuristic::compute_heuristic(const GlobalState &global_state) {
          State state = convert_global_state(global_state);
          set_preferred(task->get_operators()[42]);
          int sum = 0;
          for (FactProxy fact : state)
              sum += fact.get_value();
          return sum;
      }

  There is one additional conversion: heuristics may need to convert
  states between different tasks. For this they can use
  TaskProxy::convert_ancestor_state() which takes a state of the
  ancestor task and returns the corresponding state of the descendent
  task.

  For helper functions that work on task related objects, please see the
  task_properties.h module.
*/


/*
  Basic iterator support for proxy collections.
*/
template<typename ProxyCollection>
class ProxyIterator {
    /* We store a pointer to collection instead of a reference
       because iterators have to be copy assignable. */
    const ProxyCollection *collection;
    std::size_t pos;
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = typename ProxyCollection::ItemType;
    using difference_type = int;
    using pointer = const value_type *;
    using reference = value_type;

    ProxyIterator(const ProxyCollection &collection, std::size_t pos)
        : collection(&collection), pos(pos) {
    }

    reference operator*() const {
        return (*collection)[pos];
    }

    value_type operator++(int) {
        value_type value(**this);
        ++(*this);
        return value;
    }

    ProxyIterator &operator++() {
        ++pos;
        return *this;
    }

    bool operator==(const ProxyIterator &other) const {
        assert(collection == other.collection);
        return pos == other.pos;
    }

    bool operator!=(const ProxyIterator &other) const {
        return !(*this == other);
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
    const AbstractTask *task;
    FactPair fact;
public:
    FactProxy(const AbstractTask &task, int var_id, int value);
    FactProxy(const AbstractTask &task, const FactPair &fact);
    ~FactProxy() = default;

    VariableProxy get_variable() const;

    int get_value() const {
        return fact.value;
    }

    FactPair get_pair() const {
        return fact;
    }

    std::string get_name() const {
        return task->get_fact_name(fact);
    }

    bool operator==(const FactProxy &other) const {
        assert(task == other.task);
        return fact == other.fact;
    }

    bool operator!=(const FactProxy &other) const {
        return !(*this == other);
    }

    bool is_mutex(const FactProxy &other) const {
        return task->are_facts_mutex(fact, other.fact);
    }
};


class FactsProxyIterator {
    const AbstractTask *task;
    int var_id;
    int value;
public:
    FactsProxyIterator(const AbstractTask &task, int var_id, int value)
        : task(&task), var_id(var_id), value(value) {}
    ~FactsProxyIterator() = default;

    FactProxy operator*() const {
        return FactProxy(*task, var_id, value);
    }

    FactsProxyIterator &operator++() {
        assert(var_id < task->get_num_variables());
        int num_facts = task->get_variable_domain_size(var_id);
        assert(value < num_facts);
        ++value;
        if (value == num_facts) {
            ++var_id;
            value = 0;
        }
        return *this;
    }

    bool operator==(const FactsProxyIterator &other) const {
        assert(task == other.task);
        return var_id == other.var_id && value == other.value;
    }

    bool operator!=(const FactsProxyIterator &other) const {
        return !(*this == other);
    }
};


/*
  Proxy class for the collection of all facts of a task.

  We don't implement size() because it would not be constant-time.

  FactsProxy supports iteration, e.g. for range-based for loops. This
  iterates over all facts in order of increasing variable ID, and in
  order of increasing value for each variable.
*/
class FactsProxy {
    const AbstractTask *task;
public:
    explicit FactsProxy(const AbstractTask &task)
        : task(&task) {}
    ~FactsProxy() = default;

    FactsProxyIterator begin() const {
        return FactsProxyIterator(*task, 0, 0);
    }

    FactsProxyIterator end() const {
        return FactsProxyIterator(*task, task->get_num_variables(), 0);
    }
};


class ConditionsProxy {
protected:
    const AbstractTask *task;
public:
    using ItemType = FactProxy;
    explicit ConditionsProxy(const AbstractTask &task)
        : task(&task) {}
    virtual ~ConditionsProxy() = default;

    virtual std::size_t size() const = 0;
    virtual FactProxy operator[](std::size_t index) const = 0;

    bool empty() const {
        return size() == 0;
    }
};


class VariableProxy {
    const AbstractTask *task;
    int id;
public:
    VariableProxy(const AbstractTask &task, int id)
        : task(&task), id(id) {}
    ~VariableProxy() = default;

    bool operator==(const VariableProxy &other) const {
        assert(task == other.task);
        return id == other.id;
    }

    bool operator!=(const VariableProxy &other) const {
        return !(*this == other);
    }

    int get_id() const {
        return id;
    }

    std::string get_name() const {
        return task->get_variable_name(id);
    }

    int get_domain_size() const {
        return task->get_variable_domain_size(id);
    }

    FactProxy get_fact(int index) const {
        assert(index < get_domain_size());
        return FactProxy(*task, id, index);
    }

    bool is_derived() const {
        int axiom_layer = task->get_variable_axiom_layer(id);
        return axiom_layer != -1;
    }

    int get_axiom_layer() const {
        int axiom_layer = task->get_variable_axiom_layer(id);
        /*
          This should only be called for derived variables.
          Non-derived variables have axiom_layer == -1.
          Use var.is_derived() to check.
        */
        assert(axiom_layer >= 0);
        return axiom_layer;
    }

    int get_default_axiom_value() const {
        assert(is_derived());
        return task->get_variable_default_axiom_value(id);
    }
};


class VariablesProxy {
    const AbstractTask *task;
public:
    using ItemType = VariableProxy;
    explicit VariablesProxy(const AbstractTask &task)
        : task(&task) {}
    ~VariablesProxy() = default;

    std::size_t size() const {
        return task->get_num_variables();
    }

    VariableProxy operator[](std::size_t index) const {
        assert(index < size());
        return VariableProxy(*task, index);
    }

    FactsProxy get_facts() const {
        return FactsProxy(*task);
    }
};


class PreconditionsProxy : public ConditionsProxy {
    int op_index;
    bool is_axiom;
public:
    PreconditionsProxy(const AbstractTask &task, int op_index, bool is_axiom)
        : ConditionsProxy(task), op_index(op_index), is_axiom(is_axiom) {}
    ~PreconditionsProxy() = default;

    std::size_t size() const override {
        return task->get_num_operator_preconditions(op_index, is_axiom);
    }

    FactProxy operator[](std::size_t fact_index) const override {
        assert(fact_index < size());
        return FactProxy(*task, task->get_operator_precondition(
                             op_index, fact_index, is_axiom));
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
    ~EffectConditionsProxy() = default;

    std::size_t size() const override {
        return task->get_num_operator_effect_conditions(op_index, eff_index, is_axiom);
    }

    FactProxy operator[](std::size_t index) const override {
        assert(index < size());
        return FactProxy(*task, task->get_operator_effect_condition(
                             op_index, eff_index, index, is_axiom));
    }
};


class EffectProxy {
    const AbstractTask *task;
    int op_index;
    int eff_index;
    bool is_axiom;
public:
    EffectProxy(const AbstractTask &task, int op_index, int eff_index, bool is_axiom)
        : task(&task), op_index(op_index), eff_index(eff_index), is_axiom(is_axiom) {}
    ~EffectProxy() = default;

    EffectConditionsProxy get_conditions() const {
        return EffectConditionsProxy(*task, op_index, eff_index, is_axiom);
    }

    FactProxy get_fact() const {
        return FactProxy(*task, task->get_operator_effect(
                             op_index, eff_index, is_axiom));
    }
};


class EffectsProxy {
    const AbstractTask *task;
    int op_index;
    bool is_axiom;
public:
    using ItemType = EffectProxy;
    EffectsProxy(const AbstractTask &task, int op_index, bool is_axiom)
        : task(&task), op_index(op_index), is_axiom(is_axiom) {}
    ~EffectsProxy() = default;

    std::size_t size() const {
        return task->get_num_operator_effects(op_index, is_axiom);
    }

    EffectProxy operator[](std::size_t eff_index) const {
        assert(eff_index < size());
        return EffectProxy(*task, op_index, eff_index, is_axiom);
    }
};


class OperatorProxy {
    const AbstractTask *task;
    int index;
    bool is_an_axiom;
public:
    OperatorProxy(const AbstractTask &task, int index, bool is_axiom)
        : task(&task), index(index), is_an_axiom(is_axiom) {}
    ~OperatorProxy() = default;

    bool operator==(const OperatorProxy &other) const {
        assert(task == other.task);
        return index == other.index && is_an_axiom == other.is_an_axiom;
    }

    bool operator!=(const OperatorProxy &other) const {
        return !(*this == other);
    }

    PreconditionsProxy get_preconditions() const {
        return PreconditionsProxy(*task, index, is_an_axiom);
    }

    EffectsProxy get_effects() const {
        return EffectsProxy(*task, index, is_an_axiom);
    }

    int get_cost() const {
        return task->get_operator_cost(index, is_an_axiom);
    }

    bool is_axiom() const {
        return is_an_axiom;
    }

    std::string get_name() const {
        return task->get_operator_name(index, is_an_axiom);
    }

    int get_id() const {
        return index;
    }

    OperatorID get_global_operator_id() const {
        assert(!is_an_axiom);
        return task->get_global_operator_id(OperatorID(index));
    }
};


class OperatorsProxy {
    const AbstractTask *task;
public:
    using ItemType = OperatorProxy;
    explicit OperatorsProxy(const AbstractTask &task)
        : task(&task) {}
    ~OperatorsProxy() = default;

    std::size_t size() const {
        return task->get_num_operators();
    }

    bool empty() const {
        return size() == 0;
    }

    OperatorProxy operator[](std::size_t index) const {
        assert(index < size());
        return OperatorProxy(*task, index, false);
    }

    OperatorProxy operator[](OperatorID id) const {
        return (*this)[id.get_index()];
    }
};


class AxiomsProxy {
    const AbstractTask *task;
public:
    using ItemType = OperatorProxy;
    explicit AxiomsProxy(const AbstractTask &task)
        : task(&task) {}
    ~AxiomsProxy() = default;

    std::size_t size() const {
        return task->get_num_axioms();
    }

    bool empty() const {
        return size() == 0;
    }

    OperatorProxy operator[](std::size_t index) const {
        assert(index < size());
        return OperatorProxy(*task, index, true);
    }
};


class GoalsProxy : public ConditionsProxy {
public:
    explicit GoalsProxy(const AbstractTask &task)
        : ConditionsProxy(task) {}
    ~GoalsProxy() = default;

    std::size_t size() const override {
        return task->get_num_goals();
    }

    FactProxy operator[](std::size_t index) const override {
        assert(index < size());
        return FactProxy(*task, task->get_goal_fact(index));
    }
};


bool does_fire(const EffectProxy &effect, const State &state);
bool does_fire(const EffectProxy &effect, const GlobalState &state);


class State {
    const AbstractTask *task;
    std::vector<int> values;
public:
    using ItemType = FactProxy;
    State(const AbstractTask &task, std::vector<int> &&values)
        : task(&task), values(std::move(values)) {
        assert(static_cast<int>(size()) == this->task->get_num_variables());
    }
    ~State() = default;
    State(const State &) = default;

    State(State &&other)
        : task(other.task), values(std::move(other.values)) {
        other.task = nullptr;
    }

    State &operator=(const State &&other) {
        if (this != &other) {
            values = std::move(other.values);
        }
        return *this;
    }

    bool operator==(const State &other) const {
        assert(task == other.task);
        return values == other.values;
    }

    bool operator!=(const State &other) const {
        return !(*this == other);
    }

    std::size_t hash() const {
        std::hash<std::vector<int>> hasher;
        return hasher(values);
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

    inline TaskProxy get_task() const;

    const std::vector<int> &get_values() const {
        return values;
    }

    State get_successor(OperatorProxy op) const {
        if (task->get_num_axioms() > 0) {
            ABORT("State::apply currently does not support axioms.");
        }
        assert(!op.is_axiom());
        //assert(is_applicable(op, state));
        std::vector<int> new_values = values;
        for (EffectProxy effect : op.get_effects()) {
            if (does_fire(effect, *this)) {
                FactProxy effect_fact = effect.get_fact();
                new_values[effect_fact.get_variable().get_id()] = effect_fact.get_value();
            }
        }
        return State(*task, std::move(new_values));
    }

    void dump_pddl() const;
    void dump_fdr() const;
};


namespace std {
template<>
struct hash<State> {
    size_t operator()(const State &state) const {
        return state.hash();
    }
};
}


class TaskProxy {
    const AbstractTask *task;
public:
    explicit TaskProxy(const AbstractTask &task)
        : task(&task) {}
    ~TaskProxy() = default;

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

    /*
      Convert a state from an ancestor task into a state of this task.
      The given state has to belong to a task that is an ancestor of
      this task in the sense that this task is the result of a sequence
      of task transformations on the ancestor task. If this is not the
      case, the function aborts.
    */
    State convert_ancestor_state(const State &ancestor_state) const {
        TaskProxy ancestor_task_proxy = ancestor_state.get_task();
        // Create a copy of the state values for the new state.
        std::vector<int> state_values = ancestor_state.get_values();
        task->convert_state_values(state_values, ancestor_task_proxy.task);
        return State(*task, std::move(state_values));
    }

    const causal_graph::CausalGraph &get_causal_graph() const;
};


inline FactProxy::FactProxy(const AbstractTask &task, const FactPair &fact)
    : task(&task), fact(fact) {
    assert(fact.var >= 0 && fact.var < task.get_num_variables());
    assert(fact.value >= 0 && fact.value < get_variable().get_domain_size());
}

inline FactProxy::FactProxy(const AbstractTask &task, int var_id, int value)
    : FactProxy(task, FactPair(var_id, value)) {
}


inline VariableProxy FactProxy::get_variable() const {
    return VariableProxy(*task, fact.var);
}

inline TaskProxy State::get_task() const {
    return TaskProxy(*task);
}

inline bool does_fire(const EffectProxy &effect, const State &state) {
    for (FactProxy condition : effect.get_conditions()) {
        if (state[condition.get_variable()] != condition)
            return false;
    }
    return true;
}

inline bool does_fire(const EffectProxy &effect, const GlobalState &state) {
    for (FactProxy condition : effect.get_conditions()) {
        FactPair condition_pair = condition.get_pair();
        if (state[condition_pair.var] != condition_pair.value)
            return false;
    }
    return true;
}

#endif
