#ifndef ABSTRACT_STATE_ITERATOR_H
#define ABSTRACT_STATE_ITERATOR_H

#include <vector>

class AbstractStateIterator {
    const std::vector<int> ranges;
    std::vector<int> current;
    int counter;
    bool at_end;
public:
    explicit AbstractStateIterator(const std::vector<int> &ranges_);
    ~AbstractStateIterator();
    const std::vector<int> &get_current() const {
        return current;
    }
    void next();
    bool is_at_end() const {
        return at_end;
    }
    int get_counter() const {
        return counter;
    }
};

#endif
