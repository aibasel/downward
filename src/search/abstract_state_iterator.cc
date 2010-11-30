#include "abstract_state_iterator.h"

#include <cassert>
#include <vector>
using namespace std;


AbstractStateIterator::AbstractStateIterator(const vector<int> &ranges_)
    : ranges(ranges_), current(ranges.size(), 0), counter(0), at_end(false) {
}


AbstractStateIterator::~AbstractStateIterator() {
}


void AbstractStateIterator::next() {
    assert(!at_end);
    ++counter;
    assert(counter >= 0); // test against overflow
    for (size_t pos = 0; pos != ranges.size(); ++pos) {
        ++current[pos];
        if (current[pos] == ranges[pos]) {
            current[pos] = 0;
        } else {
            return;
        }
    }
    at_end = true;
}
