#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_CEGAR_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_CEGAR_H

#include "pattern_collection_generator_multiple.h"

namespace pdbs {
class PatternCollectionGeneratorMultipleCegar : public PatternCollectionGeneratorMultiple {
    const bool use_wildcard_plans;

    virtual std::string id() const override;
    virtual void initialize(const std::shared_ptr<AbstractTask> &) override {}
    virtual PatternInformation compute_pattern(
        int max_pdb_size,
        double max_time,
        const std::shared_ptr<utils::RandomNumberGenerator> &rng,
        const std::shared_ptr<AbstractTask> &task,
        const FactPair &goal,
        std::unordered_set<int> &&blacklisted_variables) override;
public:
    explicit PatternCollectionGeneratorMultipleCegar(const plugins::Options &opts);
};
}

#endif
