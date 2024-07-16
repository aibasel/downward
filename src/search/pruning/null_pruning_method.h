#ifndef PRUNING_NULL_PRUNING_METHOD_H
#define PRUNING_NULL_PRUNING_METHOD_H

#include "../pruning_method.h"

namespace null_pruning_method {
class NullPruningMethod : public PruningMethod {
    virtual void prune(
        const State &, std::vector<OperatorID> &) override {}
public:
    explicit NullPruningMethod(const utils::Verbosity verbosity);
    virtual void initialize(const std::shared_ptr<AbstractTask> &) override;
    virtual void print_statistics() const override {}
};

class TaskIndependentNullPruningMethod : public TaskIndependentPruningMethod {
public:
    TaskIndependentNullPruningMethod(
            const std::string &description,
            utils::Verbosity verbosity);

    virtual ~TaskIndependentNullPruningMethod() override = default;

    std::shared_ptr<PruningMethod> create_ts(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const override;
};
}

#endif
