#ifndef UTILS_COLLECTIONS_H
#define UTILS_COLLECTIONS_H

#include <cassert>
#include <cstddef>
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

template<class T>
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

template<class T>
void release_memory(T &collection) {
    T().swap(collection);
}
}


/*
  Basic iterator support for custom collections.

  To allow iterating over a new custom collection class C with items of
  type T, add the following methods and attribute:

    const T &C::operator[](std::size_t) const;
    std::size_t C::size() const;
    C::ItemType; // with a public "using ItemType = T;"
*/
template<class Collection>
class CollectionIterator {
    const Collection &collection;
    std::size_t pos;

public:
    CollectionIterator(const Collection &collection, std::size_t pos)
        : collection(collection), pos(pos) {
    }

    typename Collection::ItemType operator*() const {
        return collection[pos];
    }

    CollectionIterator &operator++() {
        ++pos;
        return *this;
    }

    bool operator==(const CollectionIterator &other) const {
        return &collection == &other.collection && pos == other.pos;
    }

    bool operator!=(const CollectionIterator &other) const {
        return !(*this == other);
    }
};

template<class Collection>
inline CollectionIterator<Collection> begin(Collection &collection) {
    return CollectionIterator<Collection>(collection, 0);
}

template<class Collection>
inline CollectionIterator<Collection> end(Collection &collection) {
    return CollectionIterator<Collection>(collection, collection.size());
}

#endif
