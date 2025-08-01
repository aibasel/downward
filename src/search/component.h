#ifndef COMPONENT_H
#define COMPONENT_H

#include "plan_manager.h"

#include "utils/logging.h"
#include "utils/hash.h"
#include "utils/tuples.h"

#include <memory>
#include <vector>

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
    PlanManager plan_manager;
public:
    TaskIndependentComponentBase(const std::string &description, utils::Verbosity verbosity)
        : description(description),
          verbosity(verbosity),
          log(utils::get_log_for_verbosity(verbosity)) {}
    virtual ~TaskIndependentComponentBase() = default;
    std::string get_description() const {return description;}
    PlanManager &get_plan_manager() {return plan_manager;}
};

using ComponentMap = utils::HashMap<const std::pair<const TaskIndependentComponentBase *, const std::shared_ptr<AbstractTask> *>,
                                    std::shared_ptr<Component>>;

template<typename TaskSpecificComponent>
class TaskIndependentComponent : public TaskIndependentComponentBase {
    virtual std::shared_ptr<TaskSpecificComponent> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const = 0;
public:
    explicit TaskIndependentComponent(const std::string &description,
                                      utils::Verbosity verbosity)
        : TaskIndependentComponentBase(description, verbosity) {}


    std::shared_ptr<TaskSpecificComponent> get_task_specific(
        const std::shared_ptr<AbstractTask> &task, int depth = -1) const {
        utils::g_log << std::string(depth >= 0 ? depth : 0, ' ') << "Creating " << description << " as root component..." << std::endl;
        std::unique_ptr<ComponentMap> component_map = std::make_unique<ComponentMap>();
        return get_task_specific(task, component_map, depth);
    }

    std::shared_ptr<TaskSpecificComponent> get_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map, int depth = -1) const {
        int indent = depth >= 0 ? depth : 0;
        std::shared_ptr<TaskSpecificComponent> component;
        const std::pair<const TaskIndependentComponentBase *, const std::shared_ptr<AbstractTask> *> key = std::make_pair(this, &task);
        if (component_map->count(key)) {
            log << std::string(indent, '.')
                << "Reusing task specific component '" << description
                << "'..." << std::endl;
            component = dynamic_pointer_cast<TaskSpecificComponent>(
                component_map->at(key));
        } else {
            log << std::string(indent, '.')
                << "Creating task specific component '" << description
                << "'..." << std::endl;
            component = create_task_specific(task, component_map, depth >= 0 ? depth + 1 : depth);
            component_map->emplace(key, component);
        }
        return component;
    }

};


template<typename R, typename ... Args>
std::shared_ptr<R> make_from_rev(
    std::unique_ptr<ComponentMap> &component_map,
    const std::shared_ptr<AbstractTask> &task,
	Args&&... args
    ) {
    int DEFAULT_DEPTH = -1;
    return construct_task_specific_from_tuple<R>(reverse_tuple(std::forward_as_tuple(std::forward<Args>(args)...)), task, component_map, DEFAULT_DEPTH);
}


template<typename R, typename ... Args>
std::shared_ptr<R> make_from_rev(
    int depth,
    std::unique_ptr<ComponentMap> &component_map,
    const std::shared_ptr<AbstractTask> &task,
	Args&&... args
    ) {
    return construct_task_specific_from_tuple<R>(reverse_tuple(std::forward_as_tuple(std::forward<Args>(args)...)), task, component_map, depth);
}


template<typename R, typename ... Args>
std::shared_ptr<R> specify(Args&&... args) {
return std::apply(
    [](auto&&... unpacked_args) {
        return make_from_rev<R>(
          std::forward<decltype(unpacked_args)>(unpacked_args)...);
        },
        reverse_tuple(std::forward_as_tuple(std::forward<Args>(args)...))
    );
}


template<typename R, typename TupleT>
std::shared_ptr<R> construct_task_specific_from_tuple(
	TupleT&& tuple,
    const std::shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map,
    int depth) {
    return std::apply(
    [&task, &component_map, &depth]<typename... Ts>(Ts&&... args) {
        return std::make_shared<R>(
            construct_task_specific(std::forward<Ts>(args),task, component_map, depth)...
        );
    },
    std::forward<TupleT>(tuple)
        );
}


// const shared_pointer<TI<T>>
template<typename T>
std::shared_ptr<T> construct_task_specific(
    const std::shared_ptr<TaskIndependentComponent<T>> &ptr,
    const std::shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map,
    int depth) {
    if (ptr == nullptr) {
        return nullptr;
    }
    return ptr->get_task_specific(task, component_map, depth);
}

// vectors
template<typename T>
auto construct_task_specific(
    const std::vector<T> &vec,
    const std::shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map,
    int depth) {
    std::vector<decltype(construct_task_specific(vec[0], task, component_map, depth))> result;
    result.reserve(vec.size());

    for (const auto &elem : vec) {
        result.push_back(construct_task_specific(elem, task, component_map, depth));
    }
    return result;
}

// other leafes
template<typename T>
T construct_task_specific(
    const T &elem,
    [[maybe_unused]] const std::shared_ptr<AbstractTask> &task,
    [[maybe_unused]] std::unique_ptr<ComponentMap> &component_map,
    [[maybe_unused]] int depth) {
    return elem;
}



#endif
