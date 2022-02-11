#ifndef PRUNING_LIMITED_PRUNING_H
#define PRUNING_LIMITED_PRUNING_H

#include "../pruning_method.h"

namespace options {
class OptionParser;
class Options;
}

namespace limited_pruning {
class LimitedPruning : public PruningMethod {
    std::shared_ptr<PruningMethod> pruning_method;
    const double min_required_pruning_ratio;
    const int num_expansions_before_checking_pruning_ratio;
    int num_pruning_calls;
    bool is_pruning_disabled;

    virtual void prune(
        const State &state, std::vector<OperatorID> &op_ids) override;
public:
    explicit LimitedPruning(const options::Options &opts);
    virtual void initialize(const std::shared_ptr<AbstractTask> &) override;
};
}

#endif
