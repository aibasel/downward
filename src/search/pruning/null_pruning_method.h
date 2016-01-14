#ifndef PRUNING_NULL_PRUNING_METHOD_H
#define PRUNING_NULL_PRUNING_METHOD_H

#include "../pruning_method.h"

class GlobalOperator;
class GlobalState;

namespace null_pruning_method {
class NullPruningMethod : public PruningMethod {
public:
    virtual void initialize();
    virtual void prune_operators(const GlobalState & /*state*/,
                                 std::vector<const GlobalOperator *> & /*ops*/) {}
    virtual void print_statistics() const {}
};
}

#endif
