#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_CEGAR_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_CEGAR_H

#include "pattern_collection_generator_multiple.h"

namespace pdbs {
class PatternCollectionGeneratorMultipleCegar : public PatternCollectionGeneratorMultiple {
    // Options for the CEGAR algorithm.
    const int max_pdb_size;
    const double cegar_max_time;
    const bool use_wildcard_plans;
protected:
    virtual std::string get_name() const override;
    virtual void initialize(const std::shared_ptr<AbstractTask> &) override {}
    virtual PatternInformation compute_pattern(
        const std::shared_ptr<AbstractTask> &task,
        FactPair goal,
        std::unordered_set<int> &&blacklisted_variables,
        const utils::CountdownTimer &timer,
        int remaining_collection_size) override;
public:
    explicit PatternCollectionGeneratorMultipleCegar(options::Options &opts);
};
}

#endif
