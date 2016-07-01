#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_TOTAL_ORDER_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_TOTAL_ORDER_H

#include "merge_scoring_function.h"

namespace options {
class OptionParser;
class Options;
}

namespace utils {
class RandomNumberGenerator;
}

namespace merge_and_shrink {
class TransitionSystem;

enum class AtomicTSOrder {
    REGULAR,
    INVERSE,
    RANDOM
};

enum class ProductTSOrder {
    OLD_TO_NEW,
    NEW_TO_OLD,
    RANDOM
};

class MergeScoringFunctionTotalOrder : public MergeScoringFunction {
    AtomicTSOrder atomic_ts_order;
    ProductTSOrder product_ts_order;
    bool atomic_before_product;
    int random_seed; // only for dump options
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    std::vector<std::pair<int, int>> merge_candidate_order;
protected:
    virtual std::string name() const override {
        return "total order";
    }
    virtual void dump_specific_options() const override;
public:
    explicit MergeScoringFunctionTotalOrder(const options::Options &options);
    virtual std::vector<int> compute_scores(
        FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) override;
    virtual void initialize(std::shared_ptr<AbstractTask> task) override;
    static void add_options_to_parser(options::OptionParser &parser);
};
}

#endif
