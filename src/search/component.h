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
};

/*
  Base class of all task-independent components of a specific type
  (e.g. Evaluator).
*/
template<internals::TaskSpecificType ComponentType>
class TaskIndependentComponent : public TaskIndependentComponentBase {
    virtual std::shared_ptr<ComponentType> create_task_specific_component(
        const std::shared_ptr<AbstractTask> &task, Cache &cache) const = 0;

public:
    using BoundType = std::shared_ptr<ComponentType>;

    BoundType bind_task(
        const std::shared_ptr<AbstractTask> &task, Cache &cache) const {
        BoundType component;
        const CacheKey key = std::make_pair(this, task.get());
        if (cache.count(key)) {
            std::shared_ptr<TaskSpecificComponent> entry = cache.at(key);
            component = std::dynamic_pointer_cast<ComponentType>(entry);
            assert(component);
        } else {
            component = create_task_specific_component(task, cache);
            cache.emplace(key, component);
        }
        return component;
    }

    BoundType bind_task(const std::shared_ptr<AbstractTask> &task) const {
        Cache cache;
        return bind_task(task, cache);
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
        const std::shared_ptr<AbstractTask> &task,
        Cache &cache) const override {
        internals::BoundArgs_t<Args> bound_args =
            internals::bind_task_recursively(args, task, cache);
        return plugins::make_shared_from_arg_tuples<T>(task, bound_args);
    }

public:
    explicit AutoTaskIndependentComponent(Args &&args) : args(move(args)) {
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
