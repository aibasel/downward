#ifndef COMPONENT_INTERNALS_H
#define COMPONENT_INTERNALS_H

#include "utils/hash.h"
#include "utils/language.h"
#include "utils/tuples.h"

#include <concepts>
#include <memory>
#include <vector>

class AbstractTask;

namespace components {
class TaskSpecificComponent;
class TaskIndependentComponentBase;

using CacheKey =
    std::pair<const TaskIndependentComponentBase *, const AbstractTask *>;
using Cache = utils::HashMap<CacheKey, std::shared_ptr<TaskSpecificComponent>>;

namespace internals {

template<typename T>
concept BasicType =
    std::convertible_to<T, std::string> || std::is_same_v<T, int> ||
    std::is_same_v<T, double> || std::is_same_v<T, bool> ||
    std::is_enum_v<std::remove_cvref_t<T>>;

template<typename T>
concept TaskSpecificType =
    std::derived_from<T, TaskSpecificComponent>;

template<typename T>
concept TaskIndependentType =
    std::derived_from<T, TaskIndependentComponentBase>;


template<TaskIndependentType T>
auto bind_task_recursively(
    const std::shared_ptr<T>&,
    const std::shared_ptr<AbstractTask>&,
    Cache&) -> typename T::BoundType;

template<typename T>
auto bind_task_recursively(
    const std::vector<T>&,
    const std::shared_ptr<AbstractTask>&,
    Cache&);

template<typename... Args>
auto bind_task_recursively(
    const std::tuple<Args...>&,
    const std::shared_ptr<AbstractTask>&,
    Cache&);

template<BasicType T>
auto bind_task_recursively(
    const T&,
    const std::shared_ptr<AbstractTask>&,
    Cache&);

template<typename Args>
struct BoundArgs {
    using type = decltype(bind_task_recursively(
        std::declval<Args>(), std::declval<std::shared_ptr<AbstractTask>>(),
        std::declval<Cache &>()));
};

template<typename Args, typename T>
concept ComponentArgsFor = utils::ConstructibleFromArgsTuple<
    T,
    typename utils::PrependedTuple<
        std::shared_ptr<AbstractTask>, typename BoundArgs<Args>::type>::type>;

template<typename ComponentType, typename T>
concept ComponentTypeOf =
    std::derived_from<ComponentType, TaskSpecificComponent> &&
    std::derived_from<T, ComponentType>;

template<TaskIndependentType T>
auto bind_task_recursively(
    const std::shared_ptr<T> &component,
    const std::shared_ptr<AbstractTask> &task, Cache &cache) -> typename T::BoundType {
    if (component) {
        return component->bind_task(task, cache);
    }
    return nullptr;
}

template<typename T>
auto bind_task_recursively(
    const std::vector<T> &vec, const std::shared_ptr<AbstractTask> &task,
    Cache &cache) {
    using BoundElementType = BoundArgs<T>::type;
    std::vector<BoundElementType> result;
    result.reserve(vec.size());
    for (const auto &elem : vec) {
        result.push_back(bind_task_recursively(elem, task, cache));
    }
    return result;
}

template<typename... Args>
auto bind_task_recursively(
    const std::tuple<Args...> &args, const std::shared_ptr<AbstractTask> &task,
    Cache &cache) {
    return std::apply(
        [&](const Args &...elems) {
            return std::make_tuple(
                bind_task_recursively(elems, task, cache)...);
        },
        args);
}

template<BasicType T>
auto bind_task_recursively(
    const T &t, const std::shared_ptr<AbstractTask> &, Cache &) {
    return t;
}
}
}
#endif
