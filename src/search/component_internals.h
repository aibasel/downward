#ifndef COMPONENT_INTERNALS_H
#define COMPONENT_INTERNALS_H

#include "utils/hash.h"
#include "utils/language.h"
#include "utils/tuples.h"

#include <concepts>
#include <memory>
#include <vector>

class AbstractTask;
class TaskSpecificComponent;
class TaskIndependentComponentBase;

using CacheKey =
    std::pair<const TaskIndependentComponentBase *, const AbstractTask *>;
using Cache = utils::HashMap<CacheKey, std::shared_ptr<TaskSpecificComponent>>;

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

template<typename T>
concept Bindable =
    requires(T t, const std::shared_ptr<AbstractTask> &task, Cache &cache) {
        {
            t.bind_task(task, cache)
        } -> std::convertible_to<std::shared_ptr<TaskSpecificComponent>>;
    };

template<Bindable T>
auto bind_task_recursively(
    const std::shared_ptr<T> &component,
    const std::shared_ptr<AbstractTask> &task, Cache &cache)
    -> decltype(component->bind_task(task, cache)) {
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

template<typename T>
// issue559 decide if we want this restriction
    requires(!std::convertible_to<T, std::shared_ptr<TaskSpecificComponent>>)
auto bind_task_recursively(
    const T &t, const std::shared_ptr<AbstractTask> &, Cache &) {
    return t;
}
#endif
