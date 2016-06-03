#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_DFP_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_DFP_H

#include "merge_strategy_factory.h"

#include <vector>

namespace options {
class OptionParser;
class Options;
}

namespace utils {
class RandomNumberGenerator;
}

namespace merge_and_shrink {
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

class MergeStrategyFactoryDFP : public MergeStrategyFactory {
    AtomicTSOrder atomic_ts_order;
    ProductTSOrder product_ts_order;
    bool atomic_before_product;
    bool randomized_order;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    std::vector<int> compute_ts_order(std::shared_ptr<AbstractTask> task);
protected:
    virtual void dump_strategy_specific_options() const override;
public:
    explicit MergeStrategyFactoryDFP(const options::Options &options);
    virtual ~MergeStrategyFactoryDFP() override = default;
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        std::shared_ptr<AbstractTask> task,
        FactoredTransitionSystem &fts) override;
    virtual std::string name() const override;
    static void add_options_to_parser(options::OptionParser &parser);
};
}

#endif
