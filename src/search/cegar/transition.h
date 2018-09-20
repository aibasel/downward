#ifndef CEGAR_TRANSITION_H
#define CEGAR_TRANSITION_H

namespace cegar {
class AbstractState;

struct Transition {
    int op_id;
    AbstractState *target;

    Transition(int op_id, AbstractState *state)
        : op_id(op_id),
          target(state) {
    }

    bool operator==(const Transition &other) {
        return op_id == other.op_id && target == other.target;
    }
};
}

#endif
