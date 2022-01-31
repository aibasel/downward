#ifndef PRUNING_METHOD_H
#define PRUNING_METHOD_H

#include "operator_id.h"
#include "task_proxy.h"

#include <memory>
#include <vector>

class AbstractTask;

class PruningMethod {
protected:
    std::shared_ptr<AbstractTask> task;

public:
    PruningMethod();
    virtual ~PruningMethod() = default;

    virtual void initialize(const std::shared_ptr<AbstractTask> &task);

    /* This method must not be called for goal states. This can be checked
       with assertions in derived classes. */
    virtual void prune_operators(const State &state,
                                 std::vector<OperatorID> &op_ids) = 0;

    virtual void print_statistics() const = 0;
};

#endif
