#ifndef ARRAY_CHAIN_H
#define ARRAY_CHAIN_H

#include <cassert>
#include <vector>

namespace array_chain {
const int INVALID_INDEX = -1;

using _Iterator = std::vector<int>::const_iterator;
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

class ArraySlice {
    friend class ArrayChain;
    _Iterator first;
    _Iterator last;

    ArraySlice(_Iterator first, _Iterator last)
        : first(first),
          last(last) {
    }
public:
    _Iterator begin() {
        return first;
    }

    _Iterator end() {
        return last;
    }
};

class ArrayChain {
    std::vector<Value> data;
public:
    ArrayChainIndex append(const std::vector<Value> &vec) {
        ArrayChainIndex index(data.size());
        data.insert(data.end(), vec.begin(), vec.end());
        return index;
    }

    ArraySlice get_slice(ArrayChainIndex index, int size) const {
        assert(index.position >= 0 &&
               size >= 0 &&
               index.position + size <= static_cast<int>(data.size()));
        return ArraySlice(data.begin() + index.position, data.begin() + index.position + size);
    }
};
}

#endif
