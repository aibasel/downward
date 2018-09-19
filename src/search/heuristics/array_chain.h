#ifndef ARRAY_CHAIN_H
#define ARRAY_CHAIN_H

#include <cassert>
#include <deque>
#include <vector>

namespace array_chain {
const int SENTINEL = -1;
const int INVALID_INDEX = -1;

using _Iterator = std::deque<int>::const_iterator;
using Value = int;

class ArrayChainIndex {
    friend class ArrayChain;
    int position;
    ArrayChainIndex(int position)
        : position(position) {
    }
public:
    ArrayChainIndex()
        : position(INVALID_INDEX) {
    }
};

class ArrayChainViewIterator {
    friend class ArrayChainView;

    _Iterator iter;
    bool is_end;

    ArrayChainViewIterator(_Iterator iter)
        : iter(iter),
          is_end(*iter == SENTINEL) {
    }

    ArrayChainViewIterator()
        : is_end(true) {
    }
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = Value;
    using difference_type = int;
    using pointer = const value_type *;
    using reference = value_type;

    reference operator*() const {
        assert(!is_end);
        return *iter;
    }

    ArrayChainViewIterator &operator++() {
        assert(!is_end);
        ++iter;
        if (*iter == SENTINEL)
            is_end = true;
        return *this;
    }

    value_type operator++(int) {
        value_type value(**this);
        ++(*this);
        return value;
    }

    bool operator==(const ArrayChainViewIterator &other) const {
        if (is_end)
            return other.is_end;
        else
            return !other.is_end && iter == other.iter;
    }

    bool operator!=(const ArrayChainViewIterator &other) const {
        return !(*this == other);
    }
};

class ArrayChainView {
    friend class ArrayChain;
    _Iterator iter;

    ArrayChainView(_Iterator iter)
        : iter(iter) {
    }
public:
    ArrayChainViewIterator begin() {
        return ArrayChainViewIterator(iter);
    }

    ArrayChainViewIterator end() {
        return ArrayChainViewIterator();
    }
};

class ArrayChain {
    std::deque<Value> data;
public:
    ArrayChainIndex append(const std::vector<Value> &vec) {
        ArrayChainIndex index(data.size());
        data.insert(data.end(), vec.begin(), vec.end());
        data.push_back(SENTINEL);
        return index;
    }

    ArrayChainView operator[](ArrayChainIndex index) const {
        assert(index.position >= 0 &&
               index.position < static_cast<int>(data.size()));
        return ArrayChainView(data.begin() + index.position);
    }
};
}

#endif
