#ifndef ALGORITHMS_ORDERED_SET_H
#define ALGORITHMS_ORDERED_SET_H

#include "../utils/collections.h"
#include "../utils/hash.h"
#include "../utils/rng.h"

#include <cassert>
#include <vector>

namespace ordered_set {
/*
  Combine vector and unordered_set to store a set of elements, ordered
  by insertion time.
*/
template<typename T>
class OrderedSet {
    std::vector<T> ordered_items;
    utils::HashSet<T> unordered_items;

public:
    bool empty() const {
        assert(unordered_items.size() == ordered_items.size());
        return ordered_items.empty();
    }

    int size() const {
        assert(unordered_items.size() == ordered_items.size());
        return ordered_items.size();
    }

    void clear() {
        ordered_items.clear();
        unordered_items.clear();
        assert(empty());
    }

    /*
      If item is not yet included in the set, append it to the end. If
      it is included, do nothing.
    */
    void insert(const T &item) {
        auto result = unordered_items.insert(item);
        bool inserted = result.second;
        if (inserted) {
            ordered_items.push_back(item);
        }
        assert(unordered_items.size() == ordered_items.size());
    }

    bool contains(const T &item) const {
        return unordered_items.count(item) != 0;
    }

    void shuffle(utils::RandomNumberGenerator &rng) {
        rng.shuffle(ordered_items);
    }

    const T &operator[](int pos) const {
        assert(utils::in_bounds(pos, ordered_items));
        return ordered_items[pos];
    }

    const std::vector<T> &get_as_vector() const {
        return ordered_items;
    }

    std::vector<T> pop_as_vector() {
        std::vector<T> items = std::move(ordered_items);
        unordered_items.clear();
        assert(empty());
        return items;
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
