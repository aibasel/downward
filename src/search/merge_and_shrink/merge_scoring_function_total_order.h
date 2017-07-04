#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_TOTAL_ORDER_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_TOTAL_ORDER_H

#include "merge_scoring_function.h"

#include <memory>

namespace options {
class OptionParser;
class Options;
}

namespace utils {
class RandomNumberGenerator;
}

namespace merge_and_shrink {
class MergeScoringFunctionTotalOrder : public MergeScoringFunction {
    enum class AtomicTSOrder {
        REVERSE_LEVEL,
        LEVEL,
        RANDOM
    };
    AtomicTSOrder atomic_ts_order;
    enum class ProductTSOrder {
        OLD_TO_NEW,
        NEW_TO_OLD,
        RANDOM
    };
    ProductTSOrder product_ts_order;
    bool atomic_before_product;
    int random_seed; // only for dump options
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    std::vector<std::pair<int, int>> merge_candidate_order;
protected:
    virtual std::string name() const override;
    virtual void dump_function_specific_options() const override;
public:
    explicit MergeScoringFunctionTotalOrder(const options::Options &options);
    virtual ~MergeScoringFunctionTotalOrder() override = default;
    virtual std::vector<double> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) override;
    virtual void initialize(const TaskProxy &task_proxy) override;
    static void add_options_to_parser(options::OptionParser &parser);
};
}

#endif
