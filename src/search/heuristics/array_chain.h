#ifndef ARRAY_CHAIN_H
#define ARRAY_CHAIN_H

#include <deque>
#include <vector>

namespace array_chain {
const int SENTINEL = -1;
const int INVALID_INDEX = -1;

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

class ArrayChain {
    std::deque<int> data;
public:
    ArrayChainIndex append(const std::vector<int> &vec);

    // TODO: Use a better type than this; some kind of view.
    std::vector<int> operator[](ArrayChainIndex index) const;
};
}

#endif
