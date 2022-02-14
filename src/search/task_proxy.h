#ifndef TASK_PROXY_H
#define TASK_PROXY_H

#include "abstract_task.h"
#include "operator_id.h"
#include "state_id.h"
#include "task_id.h"

#include "algorithms/int_packer.h"
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
class StateRegistry;
class TaskProxy;
class VariableProxy;
class VariablesProxy;

namespace causal_graph {
class CausalGraph;
}

using PackedStateBin = int_packer::IntPacker::Bin;

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
          utils::g_log << op.get_name() << endl;

  Since proxy classes only store a reference to the AbstractTask and
  some indices, they can be copied cheaply.

  In addition to the lightweight proxy classes, the task interface
  consists of the State class, which is used to hold state information
  for TaskProxy tasks. The State class contains packed or unpacked state data
  and shares the ownership with its copies, so it is cheap to copy but
  expensive to create. If performance is absolutely critical, the values of a
  state can be unpacked and accessed as a vector<int>.

  For now, heuristics work with a TaskProxy that can represent a transformed
  view of the original task. The search algorithms work on the unmodified root
  task. We therefore need to do two conversions between the search and the
  heuristics: converting states of the root task to states of the task used in
  the heuristic computation and converting operators of the task used by the
  heuristic to operators of the task used by the search for reporting preferred
  operators.
  These conversions are done by the Heuristic base class with
  Heuristic::convert_ancestor_state() and Heuristic::set_preferred().

      int FantasyHeuristic::compute_heuristic(const State &ancestor_state) {
          State state = convert_ancestor_state(ancestor_state);
          set_preferred(task->get_operators()[42]);
          int sum = 0;
          for (FactProxy fact : state)
              sum += fact.get_value();
          return sum;
      }

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

    /*
      Eventually, this method should perhaps not be part of OperatorProxy but
      live in a class that handles the task transformation and known about both
      the original and the transformed task.
    */
    OperatorID get_ancestor_operator_id(const AbstractTask *ancestor_task) const {
        assert(!is_an_axiom);
        return OperatorID(task->convert_operator_index(index, ancestor_task));
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


class State {
    /*
      TODO: We want to try out two things:
        1. having StateID and num_variables next to each other, so that they
           can fit into one 8-byte-aligned block.
        2. removing num_variables altogether, getting it on demand.
           It is not used in (very) performance-critical code.
      Perhaps 2) has no positive influence after 1) because we would just have
      a 4-byte gap at the end of the object. But it would probably be nice to
      have fewer attributes regardless.
    */
    const AbstractTask *task;
    const StateRegistry *registry;
    StateID id;
    const PackedStateBin *buffer;
    /*
      values is mutable because we think of it as a redundant representation
      of the state's contents, a kind of cache. One could argue for doing this
      differently (for example because some methods require unpacked data, so
      in some sense its presence changes the "logical state" from the program
      perspective. It is a bit weird to have a const function like unpack that
      is only called for its side effect on the object. But we decided to use
      const here to mean "const from the perspective of the state space
      semantics of the state".
    */
    mutable std::shared_ptr<std::vector<int>> values;
    const int_packer::IntPacker *state_packer;
    int num_variables;
public:
    using ItemType = FactProxy;

    // Construct a registered state with only packed data.
    State(const AbstractTask &task, const StateRegistry &registry, StateID id,
          const PackedStateBin *buffer);
    // Construct a registered state with packed and unpacked data.
    State(const AbstractTask &task, const StateRegistry &registry, StateID id,
          const PackedStateBin *buffer, std::vector<int> &&values);
    // Construct a state with only unpacked data.
    State(const AbstractTask &task, std::vector<int> &&values);

    bool operator==(const State &other) const;
    bool operator!=(const State &other) const;

    /* Generate unpacked data if it is not available yet. Calling the function
       on a state that already has unpacked data has no effect. */
    void unpack() const;

    std::size_t size() const;
    FactProxy operator[](std::size_t var_id) const;
    FactProxy operator[](VariableProxy var) const;

    TaskProxy get_task() const;

    /* Return a pointer to the registry in which this state is registered.
       If the state is not registered, return nullptr. */
    const StateRegistry *get_registry() const;
    /* Return the ID of the state within its registry. If the state is not
       registered, return StateID::no_state. */
    StateID get_id() const;

    /* Access the unpacked values. Accessing the unpacked values in a state
       that doesn't have them is an error. Use unpack() to ensure the data
       exists. */
    const std::vector<int> &get_unpacked_values() const;

    /* Access the packed values. Accessing packed values on states that do
       not have them (unregistered states) is an error. */
    const PackedStateBin *get_buffer() const;

    /*
      Create a successor state with the given operator. The operator is assumed
      to be applicable and the precondition is not checked. This will create an
      unpacked, unregistered successor. If you need registered successors, use
      the methods of StateRegistry.
      Using this method on states without unpacked values is an error. Use
      unpack() to ensure the data exists.
    */
    State get_unregistered_successor(const OperatorProxy &op) const;
};


namespace utils {
inline void feed(HashState &hash_state, const State &state) {
    /*
      Hashing a state without unpacked data will result in an error.
      We don't want to unpack states implicitly, so this rules out the option
      of unpacking the states here on demand. Mixing hashes from packed and
      unpacked states would lead to logically equal states with different
      hashes. Hashing packed (and therefore registered) states also seems like
      a performance error because it's much cheaper to hash the state IDs
      instead.
    */
    feed(hash_state, state.get_unpacked_values());
}
}


class TaskProxy {
    const AbstractTask *task;
public:
    explicit TaskProxy(const AbstractTask &task)
        : task(&task) {}
    ~TaskProxy() = default;

    TaskID get_id() const {
        return TaskID(task);
    }

    void subscribe_to_task_destruction(subscriber::Subscriber<AbstractTask> *subscriber) const {
        task->subscribe(subscriber);
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

    State create_state(std::vector<int> &&state_values) const {
        return State(*task, std::move(state_values));
    }

    // This method is meant to be called only by the state registry.
    State create_state(
        const StateRegistry &registry, StateID id,
        const PackedStateBin *buffer) const {
        return State(*task, registry, id, buffer);
    }

    // This method is meant to be called only by the state registry.
    State create_state(
        const StateRegistry &registry, StateID id,
        const PackedStateBin *buffer, std::vector<int> &&state_values) const {
        return State(*task, registry, id, buffer, std::move(state_values));
    }

    State get_initial_state() const {
        return create_state(task->get_initial_state_values());
    }

    /*
      Convert a state from an ancestor task into a state of this task.
      The given state has to belong to a task that is an ancestor of
      this task in the sense that this task is the result of a sequence
      of task transformations on the ancestor task. If this is not the
      case, the function aborts.

      Eventually, this method should perhaps not be part of TaskProxy but live
      in a class that handles the task transformation and knows about both the
      original and the transformed task.
    */
    State convert_ancestor_state(const State &ancestor_state) const {
        TaskProxy ancestor_task_proxy = ancestor_state.get_task();
        // Create a copy of the state values for the new state.
        ancestor_state.unpack();
        std::vector<int> state_values = ancestor_state.get_unpacked_values();
        task->convert_ancestor_state_values(
            state_values, ancestor_task_proxy.task);
        return create_state(std::move(state_values));
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

inline bool does_fire(const EffectProxy &effect, const State &state) {
    for (FactProxy condition : effect.get_conditions()) {
        if (state[condition.get_variable()] != condition)
            return false;
    }
    return true;
}

inline bool State::operator==(const State &other) const {
    assert(task == other.task);
    if (registry != other.registry) {
        std::cerr << "Comparing registered states with unregistered states "
                  << "or registered states from different registries is "
                  << "treated as an error because it is likely not "
                  << "intentional."
                  << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    if (registry) {
        // Both states are registered and from the same registry.
        return id == other.id;
    } else {
        // Both states are unregistered.
        assert(values);
        assert(other.values);
        return *values == *other.values;
    }
}

inline bool State::operator!=(const State &other) const {
    return !(*this == other);
}

inline void State::unpack() const {
    if (!values) {
        int num_variables = size();
        /*
          A micro-benchmark in issue348 showed that constructing the vector
          in the required size and then assigning values was faster than the
          more obvious reserve/push_back. Although, the benchmark did not
          profile this specific code.

          We might consider a bulk-unpack method in state_packer that could be
          more efficient. (One can imagine state packer to have extra data
          structures that exploit sequentially unpacking each entry, by doing
          things bin by bin.)
        */
        values = std::make_shared<std::vector<int>>(num_variables);
        for (int var = 0; var < num_variables; ++var) {
            (*values)[var] = state_packer->get(buffer, var);
        }
    }
}

inline std::size_t State::size() const {
    return num_variables;
}

inline FactProxy State::operator[](std::size_t var_id) const {
    assert(var_id < size());
    if (values) {
        return FactProxy(*task, var_id, (*values)[var_id]);
    } else {
        assert(buffer);
        assert(state_packer);
        return FactProxy(*task, var_id, state_packer->get(buffer, var_id));
    }
}

inline FactProxy State::operator[](VariableProxy var) const {
    return (*this)[var.get_id()];
}

inline TaskProxy State::get_task() const {
    return TaskProxy(*task);
}

inline const StateRegistry *State::get_registry() const {
    return registry;
}

inline StateID State::get_id() const {
    return id;
}

inline const PackedStateBin *State::get_buffer() const {
    /*
      TODO: we should profile what happens if we #ifndef NDEBUG this test here
      and in other places (e.g. the next method). The 'if' itself is probably
      not costly, but the 'cerr <<' stuff might prevent inlining.
    */
    if (!buffer) {
        std::cerr << "Accessing the packed values of an unregistered state is "
                  << "treated as an error."
                  << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    return buffer;
}

inline const std::vector<int> &State::get_unpacked_values() const {
    if (!values) {
        std::cerr << "Accessing the unpacked values of a state without "
                  << "unpacking them first is treated as an error. Please "
                  << "use State::unpack first."
                  << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    return *values;
}
#endif
