#ifndef HEURISTICS_ARRAY_POOL_H
#define HEURISTICS_ARRAY_POOL_H

#include <cassert>
#include <vector>

/*
  ArrayPool is intended as a compact representation of a large collection of
  arrays that are allocated individually but deallocated together.

  Each array may have a different size, but ArrayPool does not keep track of
  the array sizes; its user must maintain this information themselves. See the
  relaxation heuristics for usage examples.

  If the class turns out to be more generally useful, it could be templatized
  (currently, ValueType = int is hardcoded) and moved to the algorithms
  directory.
*/
namespace array_pool {
const int INVALID_INDEX = -1;

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
public:
    using Iterator = std::vector<Value>::const_iterator;
    Iterator begin() {
        return first;
    }

    Iterator end() {
        return last;
    }
private:
    friend class ArrayPool;

    Iterator first;
    Iterator last;

    ArrayPoolSlice(Iterator first, Iterator last)
        : first(first),
          last(last) {
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
