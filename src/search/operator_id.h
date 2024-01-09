#ifndef OPERATOR_ID_H
#define OPERATOR_ID_H

#include "utils/hash.h"

#include <iostream>

/*
  OperatorIDs are used to define an operator that belongs to a given
  planning task. These IDs are meant to be compact and efficient to use.
  They can be thought of as a type-safe replacement for "int" for the
  purpose of referring to an operator.

  Because of their efficiency requirements, they do *not* store which
  task they belong to, and it is the user's responsibility not to mix
  OperatorIDs that belong to different tasks.

  OperatorIDs can only refer to *operators*, not to *axioms*. This is
  by design: using OperatorID clearly communicates that only operators
  are appropriate in a given place and it is an error to use an axiom.
  We also considered introducing a class that can refer to operators or
  axioms (suggested names were OperatorOrAxiomID and ActionID, introducing
  the convention that "action" stands for "operator or axiom"), but so
  far we have not found a use case for it.
*/
class OperatorID {
    int index;

public:
    explicit OperatorID(int index)
        : index(index) {
    }

    static const OperatorID no_operator;

    int get_index() const {
        return index;
    }

    bool operator==(const OperatorID &other) const {
        return index == other.index;
    }

    bool operator!=(const OperatorID &other) const {
        return !(*this == other);
    }

    bool operator<(const OperatorID &other) const {
        return index < other.index;
    }

    int hash() const {
        return index;
    }
};

std::ostream &operator<<(std::ostream &os, OperatorID id);

namespace utils {
inline void feed(HashState &hash_state, OperatorID id) {
    feed(hash_state, id.hash());
}
}

#endif
