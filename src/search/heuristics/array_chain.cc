#include "array_chain.h"

using namespace std;

namespace array_chain {
ArrayChainIndex ArrayChain::append(const std::vector<int> &vec) {
    ArrayChainIndex index(data.size());
    data.insert(data.end(), vec.begin(), vec.end());
    data.push_back(SENTINEL);
    return index;
}

vector<int> ArrayChain::operator[](ArrayChainIndex index) const {
    vector<int> result;
    int pos = index.position;
    while (data[pos] != SENTINEL)
        result.push_back(data[pos++]);
    return result;
}
}
