#ifndef FAST_DOWNWARD_COMPONENT_MAP_H
#define FAST_DOWNWARD_COMPONENT_MAP_H

#include "abstract_task.h"

#include "plugins/any.h"

#include <unordered_map>

class ComponentMap;

class Component {
};

class TaskIndependentComponent {
public:
    explicit TaskIndependentComponent() {}
    virtual ~TaskIndependentComponent() = default;

    virtual std::shared_ptr<Component> create_task_specific_Component(std::shared_ptr<AbstractTask> &task);

    virtual std::shared_ptr<Component> create_task_specific_Component(
        std::shared_ptr<AbstractTask> &task,
        std::shared_ptr<ComponentMap> &component_map) = 0;
};


using KEY = std::pair<std::shared_ptr<AbstractTask>, void *>;
// pointer to a task specific component
using VALUE = plugins::Any;


class ComponentMap {
private:

    /*
      We do not use std::unorderd_map<KEY, VALUE> because std::pair is not hashable.
      We do not use the utils::HashMap because it does not handle std::shared_ptr.
    */

    std::unordered_map<
        std::shared_ptr<AbstractTask>,
        std::shared_ptr<
            std::unordered_map<void *, plugins::Any>>>
    component_map;

    std::shared_ptr<std::unordered_map<void *, plugins::Any>> get_inner_map(std::shared_ptr<AbstractTask> key);

public:
    explicit ComponentMap();
    virtual ~ComponentMap() = default;
    bool contains_key(KEY key);
    VALUE get_value(KEY key);
    void add_entry(KEY key, VALUE value);

    VALUE get_dual_key_value(std::shared_ptr<AbstractTask> key1, TaskIndependentComponent *key2);
    void add_dual_key_entry(std::shared_ptr<AbstractTask> key1, TaskIndependentComponent *key2, VALUE value);
};

#endif
