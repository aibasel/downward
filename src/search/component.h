
#ifndef COMPONENT_H
#define COMPONENT_H

#include "utils/logging.h"
#include "utils/hash.h"

#include <memory>

class AbstractTask;

class Component {
public:
    virtual ~Component() = default;
};

class TaskIndependentComponentBase {
protected:
    const std::string description;
    const utils::Verbosity verbosity;
    mutable utils::LogProxy log;
public:
    TaskIndependentComponentBase( const std::string &description, utils::Verbosity verbosity)
         : description(description),
           verbosity(verbosity),
           log(utils::get_log_for_verbosity(verbosity)) {}
    virtual ~TaskIndependentComponentBase() = default;
    std::string get_description() const {return description;}
};

using ComponentMap = utils::HashMap<const std::pair<const TaskIndependentComponentBase *, const std::shared_ptr<AbstractTask> *>,
                                    std::shared_ptr<Component>>;

template<typename AbstractProduct>
class TaskIndependentComponent : public TaskIndependentComponentBase {
    virtual std::shared_ptr<AbstractProduct> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const = 0;
public:
    explicit TaskIndependentComponent(const std::string &description,
                                      utils::Verbosity verbosity)
        : TaskIndependentComponentBase(description, verbosity){}


    std::shared_ptr<AbstractProduct> get_task_specific(
        const std::shared_ptr<AbstractTask> &task, int depth = -1) const {
    utils::g_log << std::string(depth >=0 ? depth : 0, ' ') << "Creating " << description << " as root component..." << std::endl;
    std::unique_ptr<ComponentMap> component_map = std::make_unique<ComponentMap>();
    return get_task_specific(task, component_map, depth);
	}

    std::shared_ptr<AbstractProduct> get_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map, int depth = -1) const {
        std::shared_ptr<AbstractProduct> component;
        const std::pair<const TaskIndependentComponentBase *, const std::shared_ptr<AbstractTask> *> key = std::make_pair(this,&task);
        if (component_map->count(key)) {
            log << std::string(depth, '.')
                << "Reusing task specific component '" << description
                << "'..." << std::endl;
            component = dynamic_pointer_cast<AbstractProduct>(
                component_map->at(key));
        } else {
            log << std::string(depth, '.')
                << "Creating task specific component '" << description
                << "'..." << std::endl;
            component = create_task_specific(task, component_map, depth >= 0 ? depth + 1 : depth);
            component_map->emplace(key, component);
        }
        return component;
    }

};

#endif

