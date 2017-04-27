#ifndef OPERATOR_ID_H
#define OPERATOR_ID_H

#include <iostream>

/*
  Operator IDs identify operators as opposed to action IDs that can
  identify operators or axioms. The task they refer to is not stored,
  so it is the user's responsibility to avoid mixing IDs from
  different tasks.
*/
class OperatorID {
    int index;

public:
    explicit OperatorID(int index)
        : index(index) {
    }

    int get_index() {
        return index;
    }

    bool operator==(const OperatorID &other) const {
        return index == other.index;
    }

    bool operator!=(const OperatorID &other) const {
        return !(*this == other);
    }

    size_t hash() const {
        return index;
    }
};

std::ostream &operator<<(std::ostream &os, OperatorID id);

namespace std {
template<>
struct hash<OperatorID> {
    size_t operator()(OperatorID id) const {
        return id.hash();
    }
};
}

#endif
