#ifndef FAST_DOWNWARD_COMPONENT_MAP_H
#define FAST_DOWNWARD_COMPONENT_MAP_H

#include "abstract_task.h"
#include "plugins/any.h"


#include <unordered_map>


using KEY = std::pair<std::shared_ptr<AbstractTask>,void*>;
using VALUE = plugins::Any; // pointer to a task specific component


class ComponentMap {
private:

    /*
     * We do not use std::unorderd_map<KEY, VALUE> because std::pair is not hashable.
     * We do not use the utils::HashMap because it does not handle std::shared_ptr.
     */

    std::shared_ptr<std::unordered_map<
                            std::shared_ptr<AbstractTask>,
                                    std::shared_ptr<
                            std::unordered_map<void*, plugins::Any>>>>
                     component_map;

    std::shared_ptr<std::unordered_map<void*, plugins::Any>> get_inner_map(std::shared_ptr<AbstractTask> key);

public:
    explicit ComponentMap(){
    }
    bool contains_key(KEY key);
    VALUE get_value(KEY key);
    void add_entry(KEY key, VALUE value);
};


#endif //FAST_DOWNWARD_COMPONENT_MAP_H
