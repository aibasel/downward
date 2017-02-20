#ifndef UTILS_COLLECTIONS_H
#define UTILS_COLLECTIONS_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <unordered_map>
#include <vector>

namespace utils {
template<class T>
extern bool is_sorted_unique(const std::vector<T> &values) {
    for (size_t i = 1; i < values.size(); ++i) {
        if (values[i - 1] >= values[i])
            return false;
    }
    return true;
}

template<class T>
bool in_bounds(int index, const T &container) {
    return index >= 0 && static_cast<size_t>(index) < container.size();
}

template<class T>
bool in_bounds(size_t index, const T &container) {
    return index < container.size();
}

template<typename T>
T swap_and_pop_from_vector(std::vector<T> &vec, size_t pos) {
    assert(in_bounds(pos, vec));
    T element = vec[pos];
    std::swap(vec[pos], vec.back());
    vec.pop_back();
    return element;
}

template<class T>
void release_vector_memory(std::vector<T> &vec) {
    std::vector<T>().swap(vec);
}

template<class KeyType, class ValueType>
ValueType get_value_or_default(
    const std::unordered_map<KeyType, ValueType> &dict,
    const KeyType &key,
    const ValueType &default_value) {
    auto it = dict.find(key);
    if (it != dict.end()) {
        return it->second;
    }
    return default_value;
}

template<typename ElemTo, typename Collection, typename MapFunc>
std::vector<ElemTo> map_vector(const Collection &collection, MapFunc map_func) {
    std::vector<ElemTo> transformed;
    transformed.reserve(collection.size());
    std::transform(begin(collection), end(collection),
                   std::back_inserter(transformed), map_func);
    return transformed;
}

template<typename T, typename Collection>
std::vector<T> sorted(Collection &&collection) {
    std::vector<T> vec(std::forward<Collection>(collection));
    std::sort(vec.begin(), vec.end());
    return vec;
}
}

#endif
