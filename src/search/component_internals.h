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
    std::convertible_to<std::decay_t<T>, std::string> ||
    std::is_same_v<std::decay_t<T>, int> ||
    std::is_same_v<std::decay_t<T>, double> ||
    std::is_same_v<std::decay_t<T>, bool> || std::is_enum_v<std::decay_t<T>>;

template<typename T>
concept TaskSpecificType = std::derived_from<T, TaskSpecificComponent>;

template<typename T>
concept TaskIndependentType =
    std::derived_from<T, TaskIndependentComponentBase>;

/*
  Helper to delay the evaluation of the static assertion below until the
  instantiation of the template.
*/
template<typename>
inline constexpr bool always_false_v = false;

template<typename T>
struct BoundArgs {
    static_assert(
        always_false_v<T>,
        "Unsupported type for task binding in BoundArgs<T>.");
};

template<BasicType T>
struct BoundArgs<T> {
    using type = T;
};

template<TaskIndependentType T>
struct BoundArgs<std::shared_ptr<T>> {
    using type = typename T::BoundType;
};

template<typename T>
struct BoundArgs<std::vector<T>> {
    using type = std::vector<std::decay_t<typename BoundArgs<T>::type>>;
};

template<typename... Ts>
struct BoundArgs<std::tuple<Ts...>> {
    using type = std::tuple<std::decay_t<typename BoundArgs<Ts>::type>...>;
};

template<typename T>
using BoundArgs_t = typename BoundArgs<std::decay_t<T>>::type;

template<typename Args, typename T>
concept ComponentArgsFor = utils::ConstructibleFromArgsTuple<
    T, typename utils::PrependedTuple<
           std::shared_ptr<AbstractTask>, BoundArgs_t<Args>>::type>;

template<typename ComponentType, typename T>
concept ComponentTypeOf =
    std::derived_from<ComponentType, TaskSpecificComponent> &&
    std::derived_from<T, ComponentType>;

template<TaskIndependentType T>
BoundArgs_t<std::shared_ptr<T>> bind_task_recursively(
    const std::shared_ptr<T> &component,
    const std::shared_ptr<AbstractTask> &task, Cache &cache) {
    if (component) {
        return component->bind_task(task, cache);
    }
    return nullptr;
}

template<BasicType T>
BoundArgs_t<T> bind_task_recursively(
    const T &t, const std::shared_ptr<AbstractTask> &, Cache &) {
    return t;
}

template<typename T>
BoundArgs_t<std::vector<T>> bind_task_recursively(
    const std::vector<T> &vec, const std::shared_ptr<AbstractTask> &task,
    Cache &cache) {
    BoundArgs_t<std::vector<T>> result;
    result.reserve(vec.size());
    for (const auto &elem : vec) {
        result.push_back(bind_task_recursively(elem, task, cache));
    }
    return result;
}

template<typename... Args>
BoundArgs_t<std::tuple<Args...>> bind_task_recursively(
    const std::tuple<Args...> &args, const std::shared_ptr<AbstractTask> &task,
    Cache &cache) {
    return std::apply(
        [&](const Args &...elems) {
            return std::make_tuple(
                bind_task_recursively(elems, task, cache)...);
        },
        args);
}
}
}
#endif
