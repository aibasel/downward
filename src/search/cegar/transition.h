#ifndef CEGAR_TRANSITION_H
#define CEGAR_TRANSITION_H

namespace cegar {
struct Transition {
    int op_id;
    int target_id;

    Transition(int op_id, int target_id)
        : op_id(op_id),
          target_id(target_id) {
    }

    bool operator==(const Transition &other) {
        return op_id == other.op_id && target_id == other.target_id;
    }
};
}

#endif
