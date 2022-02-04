#ifndef PRUNING_METHOD_H
#define PRUNING_METHOD_H

#include "operator_id.h"

#include "../utils/timer.h"

#include <memory>
#include <vector>

class AbstractTask;
class State;

namespace limited_pruning {
class LimitedPruning;
}

class PruningMethod {
    utils::Timer timer;
    friend class limited_pruning::LimitedPruning;

    virtual void prune_operators(
        const State &state, std::vector<OperatorID> &op_ids) = 0;
protected:
    std::shared_ptr<AbstractTask> task;
    long num_unpruned_successors_generated;
    long num_pruned_successors_generated;
public:
    PruningMethod();
    virtual ~PruningMethod() = default;
    virtual void initialize(const std::shared_ptr<AbstractTask> &task);
    void prune_op_ids(const State &state, std::vector<OperatorID> &op_ids);
    void print_statistics() const;
};

#endif
