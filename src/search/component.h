#ifndef FAST_DOWNWARD_COMPONENT_H
#define FAST_DOWNWARD_COMPONENT_H

#include "utils/logging.h"
#include "utils/hash.h"

#include <memory>

class AbstractTask;

class Component {
public:
    virtual ~Component() = default;
};

class TaskIndependentComponentBase { };

using ComponentMap = utils::HashMap<const TaskIndependentComponentBase *, std::shared_ptr<Component>>;

template<typename AbstractProduct>
class TaskIndependentComponent : public TaskIndependentComponentBase {
protected:
    const std::string description;
    const utils::Verbosity verbosity;
    mutable utils::LogProxy log;
public:
    explicit TaskIndependentComponent(const std::string &description,
                                      utils::Verbosity verbosity)
        : description(description),
          verbosity(verbosity), log(utils::get_log_for_verbosity(verbosity)) {
    }
    virtual ~TaskIndependentComponent() = default;
    std::string get_description() const {return description;}
    std::shared_ptr<AbstractProduct> get_task_specific(
        [[maybe_unused]] const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map, int depth) const {
        std::shared_ptr<AbstractProduct> task_specific_x;
        const TaskIndependentComponentBase *key = static_cast<const TaskIndependentComponentBase *>(this);
        if (component_map->count(key)) {
            log << std::string(depth, ' ')
                << "Reusing task specific component '" << description
                << "'..." << std::endl;
            task_specific_x = dynamic_pointer_cast<AbstractProduct>(component_map->at(key));
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
