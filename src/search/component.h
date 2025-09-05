#ifndef COMPONENT_H
#define COMPONENT_H

#include "plan_manager.h"

#include "utils/hash.h"
#include "utils/logging.h"

#include <memory>
#include <tuple>
#include <vector>

class AbstractTask;

class Component {
public:
    virtual ~Component() = default;
};


// Not using constexpr with std::is_base_of as this does not work for forward declarations
template <typename T, typename = void>
struct is_component : std::false_type {};

template <typename T>
struct is_component<T,
    std::void_t<decltype(static_cast<Component*>(std::declval<T*>()))>>
    : std::true_type {};



class TaskIndependentComponentBase {
protected:
    const std::string description;
    const utils::Verbosity verbosity;
    mutable utils::LogProxy log;
    PlanManager plan_manager; // only used for SearchAlgorithms
public:
    TaskIndependentComponentBase(
        const std::string &description, utils::Verbosity verbosity)
        : description(description),
          verbosity(verbosity),
          log(utils::get_log_for_verbosity(verbosity)) {
    }
    virtual ~TaskIndependentComponentBase() = default;
    std::string get_description() const {
        return description;
    }
    PlanManager &get_plan_manager() {
        return plan_manager;
    }
};

using ComponentMap = utils::HashMap<
    const std::pair<
        const TaskIndependentComponentBase *,
        const std::shared_ptr<AbstractTask> *>,
    std::shared_ptr<Component>>;

template<typename TaskSpecificComponentType>
class TaskIndependentComponentType : public TaskIndependentComponentBase {
    virtual std::shared_ptr<TaskSpecificComponentType> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map, int depth) const = 0;
public:
    TaskIndependentComponentType(
        const std::string &description, utils::Verbosity verbosity)
        : TaskIndependentComponentBase(description, verbosity) {
    }
    virtual std::shared_ptr<TaskSpecificComponentType> get_task_specific(
        const std::shared_ptr<AbstractTask> &task, int depth = -1) const = 0;
    virtual std::shared_ptr<TaskSpecificComponentType> get_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map, int depth = -1) const = 0;
};

// = = = wrapping logic = = =

template<typename T>
struct wrap_if_component {
    using type = std::conditional_t<
        is_component<T>::value, TaskIndependentComponentType<T>, T>;
};

template<typename T>
struct wrap_if_component<std::shared_ptr<T>> {
    using type = std::shared_ptr<typename wrap_if_component<T>::type>;
};

template<typename T>
struct wrap_if_component<std::vector<T>> {
    using type = std::vector<typename wrap_if_component<T>::type>;
};

template<typename T>
struct wrap_if_component<const T> {
    using type = std::add_const_t<typename wrap_if_component<T>::type>;
};

template<typename T>
using wrap_if_component_t = typename wrap_if_component<T>::type;

// iterate entries and wrap them with a TaskIndependentType
// if it is a Component
template<typename Tuple>
struct wrap_entry_wise;

template<typename... Args>
struct wrap_entry_wise<std::tuple<Args...>> {
    using type = std::tuple<wrap_if_component_t<Args>...>;
};

// receive type list, wrap them in a tuple because we only want to use them as
// the one and only field in TaskIndependentComponentFeature,
// forward it to wrap each entry one by one.
template<typename... Ts>
using WrapArgs = wrap_entry_wise<std::tuple<Ts...>>::type;

template<
    typename TaskSpecificComponentFeature, typename TaskSpecificType,
    typename FeatureArguments>
class TaskIndependentComponentFeature
    : public TaskIndependentComponentType<TaskSpecificType> {
    FeatureArguments args;

    virtual std::shared_ptr<TaskSpecificType> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const override {
        return create_task_specific_from_tuple<TaskSpecificComponentFeature>(
            args, task, component_map, depth);
    }
public:
    explicit TaskIndependentComponentFeature(FeatureArguments _args)
        : TaskIndependentComponentType<TaskSpecificType>(
              std::get<std::tuple_size<decltype(args)>::value - 2>(
                  _args), // get description (always second to last argument)
              std::get<std::tuple_size<decltype(args)>::value - 1>(
                  _args) // get verbosity (always last argument)
              ),
          args(std::move(_args)) {
    }

    std::shared_ptr<TaskSpecificType> get_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        int depth = -1) const override {
        utils::g_log << std::string(depth >= 0 ? depth : 0, ' ') << "Creating "
                     << this->description << " as root component..."
                     << std::endl;
        std::unique_ptr<ComponentMap> component_map =
            std::make_unique<ComponentMap>();
        return get_task_specific(task, component_map, depth);
    }

    std::shared_ptr<TaskSpecificType> get_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth = -1) const override {
        int indent = depth >= 0 ? depth : 0;
        std::shared_ptr<TaskSpecificComponentFeature> component;
        const std::pair<
            const TaskIndependentComponentBase *,
            const std::shared_ptr<AbstractTask> *>
            key = std::make_pair(this, &task);
        if (component_map->count(key)) {
            this->log << std::string(indent, '.')
                      << "Reusing task specific component '"
                      << this->description << "'..." << std::endl;
            component = dynamic_pointer_cast<TaskSpecificComponentFeature>(
                component_map->at(key));
        } else {
            this->log << std::string(indent, '.')
                      << "Creating task specific component '"
                      << this->description << "'..." << std::endl;
            component = dynamic_pointer_cast<TaskSpecificComponentFeature>(
                create_task_specific(
                    task, component_map, depth >= 0 ? depth + 1 : depth));
            component_map->emplace(key, component);
        }
        return component;
    }
};

template<typename R, typename TupleT>
std::shared_ptr<R> create_task_specific_from_tuple(
    TupleT &&_tuple, const std::shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map, int depth = -1) {
    return std::apply(
        [&task, &component_map, &depth]<typename... Ts>(Ts &&...args) {
            return std::make_shared<R>(get_task_specific_entry(
                std::forward<Ts>(args), task, component_map, depth)...);
        },
        std::tuple_cat(std::tuple(task), std::forward<TupleT>(_tuple)));
}

// shared_pointer<TI<T>>
template<typename T>
std::shared_ptr<T> get_task_specific_entry(
    const std::shared_ptr<TaskIndependentComponentType<T>> &ptr,
    const std::shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map, int depth) {
    if (ptr == nullptr) {
        return nullptr;
    }
    return ptr->get_task_specific(task, component_map, depth);
}

// vectors
template<typename T>
auto get_task_specific_entry(
    const std::vector<T> &vec, const std::shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map, int depth) {
    std::vector<decltype(get_task_specific_entry(
        vec[0], task, component_map, depth))>
        result;
    result.reserve(vec.size());

    for (const auto &elem : vec) {
        result.push_back(
            get_task_specific_entry(elem, task, component_map, depth));
    }
    return result;
}

// other leafes
template<typename T>
T get_task_specific_entry(
    const T &elem, [[maybe_unused]] const std::shared_ptr<AbstractTask> &task,
    [[maybe_unused]] std::unique_ptr<ComponentMap> &component_map,
    [[maybe_unused]] int depth) {
    return elem;
}

#endif
