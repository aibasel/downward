#ifndef UTILS_COLLECTIONS_H
#define UTILS_COLLECTIONS_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>

namespace utils {
template<class T>
extern void sort_unique(std::vector<T> &vec) {
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}

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
bool in_bounds(long index, const T &container) {
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

template<typename T>
int estimate_vector_bytes(int num_elements) {
    /*
      This estimate is based on a study of the C++ standard library
      that shipped with gcc around the year 2017. It does not claim to
      be accurate and may certainly be inaccurate for other compilers
      or compiler versions.
    */
    int size = 0;
    size += 2 * sizeof(void *);       // overhead for dynamic memory management
    size += sizeof(std::vector<T>);   // size of empty vector
    size += num_elements * sizeof(T); // size of actual entries
    return size;
}

template<typename T>
int _estimate_hash_table_bytes(int num_entries) {
    /*
      The same comments as for estimate_vector_bytes apply.
      Additionally, there may be alignment issues, especially on
      64-bit systems, that make this estimate too optimistic for
      certain cases.
    */

    assert(num_entries < (1 << 28));
    /*
      Having num_entries < 2^28 is necessary but not sufficient for
      the result value to not overflow. If we ever change this
      function to support larger data structures (using a size_t
      return value), we must update the list of bounds below (taken
      from the gcc library source).
    */
    int num_buckets = 0;
    const auto bounds = {
        2, 5, 11, 23, 47, 97, 199, 409, 823, 1741, 3469, 6949, 14033,
        28411, 57557, 116731, 236897, 480881, 976369, 1982627, 4026031,
        8175383, 16601593, 33712729, 68460391, 139022417, 282312799
    };

    for (int bound : bounds) {
        if (num_entries < bound) {
            num_buckets = bound;
            break;
        }
    }

    int size = 0;
    size += 2 * sizeof(void *);                            // overhead for dynamic memory management
    size += sizeof(T);                                     // empty container
    using Entry = typename T::value_type;
    size += num_entries * sizeof(Entry);                   // actual entries
    size += num_entries * sizeof(Entry *);                 // pointer to values
    size += num_entries * sizeof(void *);                  // pointer to next node
    size += num_buckets * sizeof(void *);                  // pointer to next bucket
    return size;
}

template<typename T>
int estimate_unordered_set_bytes(int num_entries) {
    // See comments for _estimate_hash_table_bytes.
    return _estimate_hash_table_bytes<std::unordered_set<T>>(num_entries);
}

template<typename Key, typename Value>
int estimate_unordered_map_bytes(int num_entries) {
    // See comments for _estimate_hash_table_bytes.
    return _estimate_hash_table_bytes<std::unordered_map<Key, Value>>(num_entries);
}

template<typename T>
class NamedVector {
    std::vector<T> elements;
    std::vector<std::string> names;
    bool _has_names;
    int reserved = 0;

    void add_name(std::string name) {
        if (!_has_names && !name.empty()) {
            _has_names = true;
            names.resize(elements.size(), "");
            names.reserve(reserved);
        }

        if (_has_names) {
            names.push_back(name);
        }
    }

public:
    NamedVector() = default;
    NamedVector(NamedVector<T> &&other) = default;

    template<typename ... _Args>
    void emplace_back(_Args && ... __args) {
        emplace_back_named("", std::forward<_Args>(__args) ...);
    }

    template<typename ... _Args>
    void emplace_back_named(std::string name, _Args && ... __args) {
        add_name(name);
        elements.emplace_back(std::forward<_Args>(__args) ...);
    }

    void push_back(const T &element, std::string name = "") {
        std::cout << "push back" << std::endl;
        add_name(name);
        elements.push_back(element);
    }

    T &operator[](int index) {
        return elements[index];
    }

    std::string get_name(int index) const {
        return _has_names ? names[index] : "";
    }

    int size() const {
        return elements.size();
    }

    void reserve(int capacity) {
        reserved = std::max(reserved, capacity);

        elements.reserve(capacity);

        if (_has_names) {
            names.reserve(capacity);
        }
    }

    bool has_names() const {
        return _has_names;
    }

    typename std::vector<T>::reference back() {
        return elements.back();
    }

    typename std::vector<T>::iterator begin() {
        return elements.begin();
    }

    typename std::vector<T>::iterator end() {
        return elements.end();
    }

    typename std::vector<T>::const_iterator begin() const {
        return elements.begin();
    }

    typename std::vector<T>::const_iterator end() const {
        return elements.end();
    }

    void clear() {
        elements.clear();
        names.clear();
    }

    void resize(int count, T value) {
        elements.resize(count, value);
        if (_has_names) {
            names.resize(count, "");
        }
    }
};
}

#endif
