#ifndef PRUNING_METHOD_H
#define PRUNING_METHOD_H

#include <vector>

class GlobalOperator;
class GlobalState;

// TODO: use the task interface for PruningMethod
class PruningMethod {
public:
    /* This method should never be called for goal states. This can be checked
       with assertions in derived classes. */
    virtual void prune_operators(const GlobalState &state,
                                 std::vector<const GlobalOperator *> &ops) = 0;
    virtual void print_statistics() const = 0;
};

#endif
