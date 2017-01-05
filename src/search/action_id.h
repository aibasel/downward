#ifndef ACTION_ID_H
#define ACTION_ID_H

#include <iostream>

/*
  Action ids identify operators or axioms. The task they refer to
  is not stored, so it is the users responsibility to avoid mixing
  ids from different tasks.
*/
class ActionID {
    friend std::ostream &operator<<(std::ostream &os, ActionID id);

    /*
      A non-negative value v identifies operator v.
      A negative value v identifies axiom (-1 - v).
    */
    int value;
    explicit ActionID(int value, bool is_axiom = false)
        : value(is_axiom? -1 - value : value) {
    }
public:
    ActionID() = delete;

    bool operator==(const ActionID &other) const {
        return value == other.value;
    }

    bool operator!=(const ActionID &other) const {
        return !(*this == other);
    }

    size_t hash() const {
        return value;
    }
};

std::ostream &operator<<(std::ostream &os, ActionID id);

namespace std {
template<>
struct hash<ActionID> {
    size_t operator()(ActionID id) const {
        return id.hash();
    }
};
}

#endif
