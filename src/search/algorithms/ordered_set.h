#ifndef ALGORITHMS_ORDERED_SET_H
#define ALGORITHMS_ORDERED_SET_H

#include "../utils/collections.h"

#include <cassert>
#include <unordered_set>
#include <vector>

namespace algorithms {
/*
  Combine vector and unordered_set to store a set of elements, ordered
  by insertion time.
*/
template <typename T>
class OrderedSet {
    std::vector<T> ordered_items;
    std::unordered_set<T> unordered_items;

public:
    bool empty() const {
        assert(unordered_items.size() == ordered_items.size());
        return ordered_items.empty();
    }

    std::size_t size() const {
        assert(unordered_items.size() == ordered_items.size());
        return ordered_items.size();
    }

    void clear() {
        utils::release_memory(ordered_items);
        utils::release_memory(unordered_items);
        assert(empty());
    }

    void add(const T &item) {
        auto result = unordered_items.insert(item);
        bool inserted = result.second;
        if (inserted) {
            ordered_items.push_back(item);
        }
        assert(unordered_items.size() == ordered_items.size());
    }

    bool contains(const T &item) const {
        return static_cast<bool>(unordered_items.count(item));
    }

    const T &operator[](std::size_t pos) const {
        assert(utils::in_bounds(pos, ordered_items));
        return ordered_items[pos];
    }

    std::vector<T> pop_as_vector() {
        std::vector<T> items = std::move(ordered_items);
        clear();
        return items;
    }

    std::unordered_set<T> pop_as_unordered_set() {
        std::vector<T> items = std::move(unordered_items);
        clear();
        return items;
    }

    std::pair<std::vector<T>, std::unordered_set<T>> pop_collections() {
        auto collections = make_pair(
            std::move(ordered_items), std::move(unordered_items));
        assert(empty());
        return collections;
    }

    typename std::vector<T>::const_iterator begin() const {
        return ordered_items.begin();
    }

    typename std::vector<T>::const_iterator end() const {
        return ordered_items.end();
    }
};
}

#endif
