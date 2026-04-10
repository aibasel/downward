#ifndef COMPONENT_H
#define COMPONENT_H

#include "component_internals.h"
#include "task_proxy.h"

#include "plugins/plugin.h"
#include "utils/tuples.h"

#include <memory>

class AbstractTask;

namespace components {
/*
  Base class for all classes that represent components bound to a specific
  task, like Evaluator, SearchAlgorithm, and OpenList.
*/
class TaskSpecificComponent {
protected:
    /*
      Hold a reference to the task implementation and pass it to objects that
      need it.
    */
    const std::shared_ptr<AbstractTask> task;

    /*
      Use task_proxy to access task information.
    */
    TaskProxy task_proxy;
public:
    explicit TaskSpecificComponent(const std::shared_ptr<AbstractTask> &task)
        : task(task), task_proxy(*task) {
    }

    virtual ~TaskSpecificComponent() = default;
};

/*
  Base class for all task-independent components. We need this non-templated
  base class to mix components of different types in the same container.
*/
class TaskIndependentComponentBase {
public:
    virtual ~TaskIndependentComponentBase() = default;

    virtual void get_task_preserving_subcomponents(
        std::vector<TaskIndependentComponentBase *> & /*components*/) const {
    }

    virtual std::shared_ptr<TaskSpecificComponent> get_cached(
        const std::shared_ptr<AbstractTask> &task) const = 0;
};

/*
  Base class of all task-independent components of a specific type
  (e.g. Evaluator).
*/
template<internals::TaskSpecificType ComponentType>
class TaskIndependentComponent : public TaskIndependentComponentBase {
    virtual std::shared_ptr<ComponentType> create_task_specific_component(
        const std::shared_ptr<AbstractTask> &task) const = 0;
protected:
    using CacheKey = const AbstractTask *;
    mutable utils::HashMap<CacheKey, std::weak_ptr<ComponentType>> cache;
public:
    using BoundType = std::shared_ptr<ComponentType>;

    /*
      Returns a task-specific component for the given task.

      The cache ensures that if the same task is bound to the same
      task-independent component multiple times during construction (for example
      when using a heuristic both for heuristic values and for preferred
      operators), the task-specific component is shared.

      The cache will not keep created components alive, though. Components are
      only reused, while a shared pointer to them exists (outside of the cache).
    */
    BoundType bind_task(const std::shared_ptr<AbstractTask> &task) const {
        BoundType component = get_cached_typed(task);
        if (!component) {
            const CacheKey key = task.get();
            component = create_task_specific_component(task);
            assert(!cache.contains(key));
            cache.emplace(key, component);
        }
        return component;
    }

    BoundType get_cached_typed(
        const std::shared_ptr<AbstractTask> &task) const {
        const CacheKey key = task.get();
        if (cache.contains(key)) {
            std::weak_ptr<ComponentType> entry = cache.at(key);
            if (entry.expired()) {
                cache.erase(key);
            } else {
                return entry.lock();
            }
        }
        return nullptr;
    }

    virtual std::shared_ptr<TaskSpecificComponent> get_cached(
        const std::shared_ptr<AbstractTask> &task) const override {
        return get_cached_typed(task);
    }
};

/*
  Templated implementation of a task-independent component. This class stores
  arguments to construct a task-specific component (e.g. HMHeuristic,
  EagerSearch) in their task-independent component form. When binding a task to
  this component, it recursively binds all these arguments to the task and
  instantiates the task-specific component.
*/
template<
    typename T, internals::ComponentTypeOf<T> ComponentType,
    internals::ComponentArgsFor<T> Args>
class AutoTaskIndependentComponent
    : public TaskIndependentComponent<ComponentType> {
    Args args;

    virtual std::shared_ptr<ComponentType> create_task_specific_component(
        const std::shared_ptr<AbstractTask> &task) const override {
        internals::BoundArgs_t<Args> bound_args =
            internals::bind_task_recursively(args, task);
        return plugins::make_shared_from_arg_tuples<T>(task, bound_args);
    }

public:
    explicit AutoTaskIndependentComponent(Args &&args) : args(move(args)) {
    }

    virtual void get_task_preserving_subcomponents(
        std::vector<TaskIndependentComponentBase *> &components) const override {
        internals::collect_task_preserving_components(args, components);
    }
};

/*
  Creates an AutoTaskIndependentComponent for type T. Instead of making this
  a construtor of the class, we use a function here, so template arguments
  can be partially inferred. We repeat the constraints of
  AutoTaskIndependentComponent here for earlier and more useful error messages.
*/
template<typename T, typename ComponentType, typename... Args>
    requires internals::ComponentTypeOf<ComponentType, T> &&
             internals::ComponentArgsFor<utils::FlatTuple_t<Args...>, T>
std::shared_ptr<TaskIndependentComponent<ComponentType>>
make_auto_task_independent_component(Args &&...args) {
    using FlatArgs = utils::FlatTuple_t<Args...>;
    using AutoComponent =
        AutoTaskIndependentComponent<T, ComponentType, FlatArgs>;
    FlatArgs flat_args =
        utils::flatten_tuple(std::make_tuple(std::forward<Args>(args)...));
    return make_shared<AutoComponent>(move(flat_args));
}
}
#endif
