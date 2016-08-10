#ifndef PRUNING_METHOD_H
#define PRUNING_METHOD_H

#include "task_proxy.h"

#include <memory>
#include <vector>

class GlobalOperator;
class GlobalState;

class PruningMethod {
    bool initialized;
public:
    PruningMethod();
    virtual ~PruningMethod() = default;

    virtual void initialize(const TaskProxy &task_proxy);

    /* This method should never be called for goal states. This can be checked
       with assertions in derived classes. */
    virtual void prune_operators(const State &state,
                                 std::vector<int> &op_ids) = 0;
    // TODO remove this overload once the search uses the task interface.
    virtual void prune_operators(const GlobalState &state,
                                 std::vector<const GlobalOperator *> &ops);
    virtual void print_statistics() const = 0;
};

#endif
