#ifndef FAST_DOWNWARD_COMPONENT_H
#define FAST_DOWNWARD_COMPONENT_H

#include "utils/logging.h"

#include <memory>
#include <unordered_map>

class AbstractTask;

class Component {
public:
    virtual ~Component() = default;
};

using UniqueKey = int;
using ComponentMap = std::unordered_map<UniqueKey, std::shared_ptr<Component>>;

static UniqueKey create_unique_key() {
    static int key = 0; //TODO unique ptr
    ++key;
    return key;
}

template <typename AbstractProduct>
class TaskIndependentComponent {
protected:
    const UniqueKey key;
    const std::string description;
    const utils::Verbosity verbosity;
    mutable utils::LogProxy log;
public:
    explicit TaskIndependentComponent(const std::string &description,
                                      utils::Verbosity verbosity)
        : key(create_unique_key()), description(description),
          verbosity(verbosity), log(utils::get_log_for_verbosity(verbosity)) {
    }
    virtual ~TaskIndependentComponent() = default;

    std::string get_description() const {return description;}
    std::shared_ptr<AbstractProduct> get_task_specific(
        [[maybe_unused]] const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map, int depth) const {
        std::shared_ptr<AbstractProduct> task_specific_x;

        if (false && component_map->count(key)) { //TODO issue559 remove "fasle &&"
            log << std::string(depth, ' ')
                << "Reusing task specific component '" << description
                << "'..." << std::endl;
            task_specific_x = std::dynamic_pointer_cast<AbstractProduct>(component_map->at(key));
        } else {
            log << std::string(depth, ' ')
                << "Creating task specific component '" << description
                << "'..." << std::endl;
            task_specific_x = create_ts(task, component_map, depth);
            component_map->emplace(key, task_specific_x);
        }
        return task_specific_x;
    }

    virtual std::shared_ptr<AbstractProduct> create_ts(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const = 0;
};

#endif
