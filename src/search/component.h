#ifndef COMPONENT_H
#define COMPONENT_H

#include "plan_manager.h"

#include "utils/logging.h"
#include "utils/hash.h"
#include "utils/tuples.h"

#include <memory>
#include <tuple>
#include <vector>

class AbstractTask;

using ComponentArgs = std::tuple<const std::string, utils::Verbosity>;
class Component {
public:
    virtual ~Component() = default;
};

template <typename T>
struct is_component {
    static constexpr bool value = std::is_base_of<Component, T>::value;
};

class TaskIndependentComponentBase {
protected:
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
    virtual std::string get_description() const = 0;
    PlanManager &get_plan_manager() {return plan_manager;}
};

using ComponentMap = utils::HashMap<const std::pair<const TaskIndependentComponentBase *, const std::shared_ptr<AbstractTask> *>,
                                    std::shared_ptr<Component>>;

template<typename TaskSpecificComponentType>
class TaskIndependentComponentType : public TaskIndependentComponentBase {
    virtual std::shared_ptr<TaskSpecificComponentType> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const = 0;
public:
    TaskIndependentComponentType()
        : TaskIndependentComponentBase("issue559.default_type_decription", utils::Verbosity::NORMAL) {}
    TaskIndependentComponentType(const std::string &description,
                                      utils::Verbosity verbosity)
        : TaskIndependentComponentBase(description, verbosity) {}
    std::string get_description() const override {return description;}
    virtual std::shared_ptr<TaskSpecificComponentType> get_task_specific(
        const std::shared_ptr<AbstractTask> &task, int depth = -1) const = 0;
    virtual std::shared_ptr<TaskSpecificComponentType> get_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map, int depth = -1) const = 0;
};

//// === FlattenTuple ===
//template <typename T>
//struct FlattenTuple {
//    using type = std::tuple<T>;
//};
//
//template <typename... Ts>
//struct FlattenTuple<std::tuple<Ts...>> {
//    using type = decltype(std::tuple_cat(typename FlattenTuple<Ts>::type{}...));
//};
//
//// type wrapper based on inheritance
//template <typename T>
//using ToTaskIndependentIfComponent = std::conditional_t<
//    is_component<T>::value,
//    TaskIndependentComponentType<T>,
//    T
//>;
//
//// Transform a tuple into one with types conditionally wrapped
//template <typename Tuple>
//struct TupleToTaskIndependent;
//
//template <typename... Ts>
//struct TupleToTaskIndependent<std::tuple<Ts...>> {
//    using type = std::tuple<ToTaskIndependentIfComponent<Ts>...>;
//};
//
////Full flattening + wrapping combined
//template <typename T>
//struct InnerTaskIndependent {
//    using Flat = typename FlattenTuple<T>::type;
//    using type = typename TupleToTaskIndependent<Flat>::type;
//};
//
//template <typename T>
//using InnerTaskIndependnet_t = typename InnerTaskIndependent<T>::type;

template<typename TaskSpecificComponentFeature, typename TaskSpecificType, typename FeatureArguments>
class TaskIndependentComponentFeature : public TaskIndependentComponentType<TaskSpecificType> {
    FeatureArguments args;
    
    virtual std::shared_ptr<TaskSpecificType> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const override {

            return construct_task_specific_from_tuple<TaskSpecificComponentFeature>(args, task, component_map, depth);
	};
public:
    explicit TaskIndependentComponentFeature(FeatureArguments _args)
        : args(std::move(_args)) {};

    //template<typename ... XArgs>
    //explicit TaskIndependentComponentFeature(XArgs&& ... args) : 
    //    TaskIndependentComponentFeature/*<TaskSpecificComponentFeature,TaskSpecificType, FeatureArguments>*/(reverse_tuple(std::forward_as_tuple(std::forward<XArgs>(args)...))) {    }

    std::shared_ptr<TaskSpecificType> get_task_specific(
        const std::shared_ptr<AbstractTask> &task, int depth = -1) const override {
        utils::g_log << std::string(depth >= 0 ? depth : 0, ' ') << "Creating " << this->description << " as root component..." << std::endl;
        std::unique_ptr<ComponentMap> component_map = std::make_unique<ComponentMap>();
        return get_task_specific(task, component_map, depth);
    }

    std::shared_ptr<TaskSpecificType> get_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map, int depth = -1) const override {
        int indent = depth >= 0 ? depth : 0;
        std::shared_ptr<TaskSpecificComponentFeature> component;
        const std::pair<const TaskIndependentComponentBase *, const std::shared_ptr<AbstractTask> *> key = std::make_pair(this, &task);
        if (component_map->count(key)) {
            this->log << std::string(indent, '.')
                << "Reusing task specific component '" << this->description
                << "'..." << std::endl;
            component = dynamic_pointer_cast<TaskSpecificComponentFeature>(
                component_map->at(key));
        } else {
            this->log << std::string(indent, '.')
                << "Creating task specific component '" << this->description
                << "'..." << std::endl;
            component = dynamic_pointer_cast<TaskSpecificComponentFeature>(create_task_specific(task, component_map, depth >= 0 ? depth + 1 : depth));
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
	TupleT&& _tuple,
    const std::shared_ptr<AbstractTask> &task,
    std::unique_ptr<ComponentMap> &component_map,
    int depth = -1) {
    return std::apply(
    [&task, &component_map, &depth]<typename... Ts>(Ts&&... args) {
        return std::make_shared<R>(
            construct_task_specific(std::forward<Ts>(args),task, component_map, depth)...
        );
    },
    std::tuple_cat(std::tuple(task),std::forward<TupleT>(_tuple)
        ));
}


// const shared_pointer<TI<T>>
template<typename T>
std::shared_ptr<T> construct_task_specific(
    const std::shared_ptr<TaskIndependentComponentType<T>> &ptr,
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
