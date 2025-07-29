
#ifndef COMPONENT_H
#define COMPONENT_H

#include "plan_manager.h"

#include "utils/logging.h"
#include "utils/hash.h"

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
    TaskIndependentComponentBase( const std::string &description, utils::Verbosity verbosity)
         : description(description),
           verbosity(verbosity),
           log(utils::get_log_for_verbosity(verbosity)) {}
    virtual ~TaskIndependentComponentBase() = default;
    std::string get_description() const {return description;}
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
        : TaskIndependentComponentBase(description, verbosity){}


    std::shared_ptr<TaskSpecificComponent> get_task_specific(
        const std::shared_ptr<AbstractTask> &task, int depth = -1) const {
    utils::g_log << std::string(depth >=0 ? depth : 0, ' ') << "Creating " << description << " as root component..." << std::endl;
    std::unique_ptr<ComponentMap> component_map = std::make_unique<ComponentMap>();
    return get_task_specific(task, component_map, depth);
	}

    std::shared_ptr<TaskSpecificComponent> get_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map, int depth = -1) const {
        std::shared_ptr<TaskSpecificComponent> component;
        const std::pair<const TaskIndependentComponentBase *, const std::shared_ptr<AbstractTask> *> key = std::make_pair(this,&task);
        if (component_map->count(key)) {
            log << std::string(depth, '.')
                << "Reusing task specific component '" << description
                << "'..." << std::endl;
            component = dynamic_pointer_cast<TaskSpecificComponent>(
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

    PlanManager &get_plan_manager() {return plan_manager;}
};




template<typename T>
struct is_task_independent : std::false_type {};

template<typename T>
constexpr bool is_task_independent_v = is_task_independent<T>::value;

// Specialization for TaskIndependentComponent class that sets it to true
template<typename T>
struct is_task_independent<TaskIndependentComponent<T>> : std::true_type {};


// const shared_pointer<TI<T>>
template<typename T>
std::shared_ptr<T> construct_task_specific(
    const std::shared_ptr<TaskIndependentComponent<T>> &ptr,
    const std::shared_ptr<AbstractTask>& task,
    std::unique_ptr<ComponentMap>& component_map,
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

//// raw pointers
//template<typename T>
//auto construct_task_specific(
//    const T* ptr,
//    const std::shared_ptr<AbstractTask>& task,
//    std::unique_ptr<ComponentMap>& component_map,
//    int depth) {
//    if (ptr == nullptr) {
//        return nullptr;
//    }
//    return &construct_task_specific(*ptr, task, component_map, depth);
//}

// const shared_pointer<TI<T>>
//template<typename T>
//std::shared_ptr<T> construct_task_specific(
//    const std::shared_ptr<TaskIndependentComponent<T>> &ptr,
//    const std::shared_ptr<AbstractTask>& task,
//    std::unique_ptr<ComponentMap>& component_map,
//    int depth) {
//    if (ptr == nullptr) {
//        return nullptr;
//    }
//    return std::make_shared<T>(std::move(construct_task_specific(*ptr, task, component_map, depth)));
//}

//// const shared_pointers
//template<typename T, typename U>
//std::shared_ptr<U> construct_task_specific(
//    const std::shared_ptr<T> &ptr,
//    const std::shared_ptr<AbstractTask>& task,
//    std::unique_ptr<ComponentMap>& component_map,
//    int depth) {
//    if (ptr == nullptr) {
//        return nullptr;
//    }
//    return construct_task_specific(*ptr, task, component_map, depth);
//}
//
//// unique_pointers
//template<typename T, typename U>
//std::unique_ptr<U> construct_task_specific(
//    const std::unique_ptr<T> ptr,
//    const std::shared_ptr<AbstractTask>& task,
//    std::unique_ptr<ComponentMap>& component_map,
//    int depth) {
//    if (ptr == nullptr) {
//        return nullptr;
//    }
//    return std::make_unique<U>(std::move(construct_task_specific(*ptr, task, component_map, depth)));
//}

//// tuples
//template<typename... Ts>
//auto construct_task_specific(
//    const std::tuple<Ts...>& tup,
//    const std::shared_ptr<AbstractTask>& task,
//    std::unique_ptr<ComponentMap>& component_map,
//    int depth) {
//    return std::apply(
//        [&task, &component_map, depth](const auto&... args) {
//            return std::make_tuple(
//                construct_task_specific(args, task, component_map, depth)...
//            );
//        },
//        tup
//    );
//}
//
//// vector<shared_ptr<TaskIndependent<T>>>
//template<typename T>
//std::vector<std::shared_ptr<T>> construct_task_specific(const std::vector<std::shared_ptr<TaskIndependentComponent<T>>>& vec,
//                           const std::shared_ptr<AbstractTask>& task,
//                           std::unique_ptr<ComponentMap>& component_map,
//                           int depth) {
//    std::vector<std::shared_ptr<T>> result;
//    result.reserve(vec.size());
//    
//    for (const std::shared_ptr<TaskIndependentComponent<T>>& elem : vec) {
//        result.emplace_back(make_shared<T>(std::move(construct_task_specific(*elem, task, component_map, depth))));
//    }
//    return result;
//}
//
//// vectors
//template<typename T, typename U>
//std::vector<U> construct_task_specific(const std::vector<T>& vec,
//                           const std::shared_ptr<AbstractTask>& task,
//                           std::unique_ptr<ComponentMap>& component_map,
//                           int depth) {
//    std::vector<U> result;
//    result.reserve(vec.size());
//    
//    for (const T& elem : vec) {
//        result.emplace_back(construct_task_specific(elem, task, component_map, depth));
//    }
//    return result;
//}
//
////// shared_ptr TI leafs
////template<typename T>
////std::shared_ptr<T> construct_task_specific(
////const std::shared_ptr<TaskIndependentComponent<T>> &elem,
////const std::shared_ptr<AbstractTask> &task,
////std::unique_ptr<ComponentMap> &component_map,
////int depth) {
////return std::make_shared<T>(std::move(elem->get_task_specific(task, component_map, depth >= 0 ? depth+1:depth)));
////}
//
//
//// TI leafs
//template<typename T>
//T construct_task_specific(
//const TaskIndependentComponent<T> &elem,
//const std::shared_ptr<AbstractTask> &task,
//std::unique_ptr<ComponentMap> &component_map,
//int depth) {
//return elem.get_task_specific(task, component_map, depth >= 0 ? depth+1:depth);
//}
//
//// other leafes
//template<typename T>
//T construct_task_specific(
//	const T &elem,
//	[[maybe_unused]] const std::shared_ptr<AbstractTask> &task,
//	[[maybe_unused]] std::unique_ptr<ComponentMap> &component_map,
//	[[maybe_unused]] int depth) {
//        return elem;
//}
////// vectors
////template<typename T>
////auto construct_task_specific(
////    const std::vector<T> &vec,
////    const std::shared_ptr<AbstractTask> &task,
////    std::unique_ptr<ComponentMap> &component_map,
////    int depth) {
////    std::vector<decltype(construct_task_specific(vec[0], task, component_map, depth))> result;
////    result.reserve(vec.size());
////    
////    for (const auto &elem : vec) {
////        result.push_back(construct_task_specific(elem, task, component_map, depth));
////    }
////    return result;
////}

/*
template <typename ... Entries>
class TaskIndependentTuple : public TaskIndependentComponent<std::tuple<Entries ...>> {
    std::tuple<Entries ...> task_independent_entries;
    
    std::shared_ptr<std::tuple<Entries ...>> create_task_specific(        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const override {

        return std::apply(
            [&task,&component_map,&depth](const auto&... task_independent_entry) {
                return std::make_tuple(get_task_specific(task_independent_entry,task,component_map,depth);
                    [&task,&component_map,&depth]<typename T >(const T& elem) {//make this lambda free stanmding func

                        if constexpr (is_task_independent_v<T>) {
                            return elem.get_task_specific(task,component_map,
						 depth >= 0 ? depth + 1 : depth
						 );
			} else { 
			    return elem;
			}
		    }(task_independent_entry)...
		);
        },
        task_independent_entries
    );
};
public:
    TaskIndependentTuple(
        const std::string &description,
        utils::Verbosity verbosity,
        std::tuple<Entries ...> task_independent_entries
	) : TaskIndependentComponentBase(description, verbosity), task_independent_entries(task_independent_entries) {};
};



template <typename EntryType>
class TaskIndependentVector : public TaskIndependentComponent<std::vector<EntryType>> {
    std::vector<EntryType> task_independent_entries;
    
    std::shared_ptr<std::vector<EntryType>> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const override {


    std::vector<std::shared_ptr<EntryType>> task_specific_entries(task_independent_entries.size());
    transform(task_independent_entries.begin(), task_independent_entries.end(), task_specific_entries.begin(),
              [&task, &component_map, &depth](const std::shared_ptr<TaskIndependentComponent<EntryType>> &entry) {
                  return entry->get_task_specific(task, component_map, depth);
              }
              );
    return task_specific_entries;
};
public:
    TaskIndependentVector(
        const std::string &description,
        utils::Verbosity verbosity,
        std::tuple<EntryType> task_independent_entries
	) : TaskIndependentComponentBase(description, verbosity), task_independent_entries(task_independent_entries) {};
};
*/


#endif

