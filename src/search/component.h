#ifndef COMPONENT_H
#define COMPONENT_H

#include "component_internals.h"

#include "task_proxy.h"

#include <memory>

class AbstractTask;

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
  Base class of all components of a specific type (e.g. Evaluator).
*/
template<typename ComponentType>
class TaskIndependentComponent : public TaskIndependentComponentBase {
    virtual std::shared_ptr<ComponentType> create_task_specific_component(
        const std::shared_ptr<AbstractTask> &task, Cache &cache) const = 0;

public:
    std::shared_ptr<ComponentType> bind_task(
        const std::shared_ptr<AbstractTask> &task, Cache &cache) const {
        std::shared_ptr<ComponentType> component;
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

    std::shared_ptr<ComponentType> bind_task(
        const std::shared_ptr<AbstractTask> &task) const {
        Cache cache;
        return bind_task(task, cache);
    }
};

#endif
