#ifndef CEGAR_ARC_H
#define CEGAR_ARC_H

class AbstractTask;

namespace cegar {
class AbstractState;

struct Arc {
    int op_id;
    AbstractState *target;

    Arc(int op_id, AbstractState *state)
        : op_id(op_id),
          target(state) {
    }

    bool operator==(const Arc &other) {
        return op_id == other.op_id && target == other.target;
    }
};
}

#endif
