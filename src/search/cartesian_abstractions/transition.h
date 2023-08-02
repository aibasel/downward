#ifndef CARTESIAN_ABSTRACTIONS_TRANSITION_H
#define CARTESIAN_ABSTRACTIONS_TRANSITION_H

#include <iostream>

namespace cartesian_abstractions {
struct Transition {
    int op_id;
    int target_id;

    Transition(int op_id, int target_id)
        : op_id(op_id),
          target_id(target_id) {
    }

    bool operator==(const Transition &other) const {
        return op_id == other.op_id && target_id == other.target_id;
    }

    friend std::ostream &operator<<(std::ostream &os, const Transition &t) {
        return os << "[" << t.op_id << "," << t.target_id << "]";
    }
};
}

#endif
