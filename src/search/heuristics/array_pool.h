#ifndef HEURISTICS_ARRAY_POOL_H
#define HEURISTICS_ARRAY_POOL_H

#include <cassert>
#include <vector>

namespace array_pool {
const int INVALID_INDEX = -1;

using _Iterator = std::vector<int>::const_iterator;
using Value = int;

class ArrayPoolIndex {
    friend class ArrayPool;
    int position;
    ArrayPoolIndex(int position)
        : position(position) {
    }
public:
    ArrayPoolIndex()
        : position(INVALID_INDEX) {
    }
};

class ArrayPoolSlice {
    friend class ArrayPool;
    _Iterator first;
    _Iterator last;

    ArrayPoolSlice(_Iterator first, _Iterator last)
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

class ArrayPool {
    std::vector<Value> data;
public:
    ArrayPoolIndex append(const std::vector<Value> &vec) {
        ArrayPoolIndex index(data.size());
        data.insert(data.end(), vec.begin(), vec.end());
        return index;
    }

    ArrayPoolSlice get_slice(ArrayPoolIndex index, int size) const {
        assert(index.position >= 0 &&
               size >= 0 &&
               index.position + size <= static_cast<int>(data.size()));
        return ArrayPoolSlice(data.begin() + index.position, data.begin() + index.position + size);
    }
};
}

#endif
