#ifndef UTILS_TUPLES_H
#define UTILS_TUPLES_H

#include <tuple>

namespace utils {
template<class ... Ts, std::size_t... Is>
auto flatten_tuple_elements(
    std::tuple<Ts...> &&t, std::index_sequence<Is...>);

template<class T>
auto flatten_tuple(T &&t) {
    return std::make_tuple(std::move(t));
}

template<class ... Ts>
auto flatten_tuple(std::tuple<Ts...> &&t) {
    constexpr std::size_t tuple_size =
        std::tuple_size<std::tuple<Ts...>>::value;
    return flatten_tuple_elements(
        std::move(t), std::make_index_sequence<tuple_size>());
}

template<class ... Ts, std::size_t... Is>
auto flatten_tuple_elements(
    std::tuple<Ts...> &&t, std::index_sequence<Is...>) {
    return std::tuple_cat(flatten_tuple(std::get<Is>(std::move(t)))...);
}
}

#endif
