#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_RANDOM_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_RANDOM_H

#include "pattern_collection_generator_multiple.h"

namespace pdbs {
class PatternCollectionGeneratorMultipleRandom : public PatternCollectionGeneratorMultiple {
    const bool bidirectional;
    std::vector<std::vector<int>> cg_neighbors;

    virtual std::string id() const override;
    virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
    virtual PatternInformation compute_pattern(
        int max_pdb_size,
        double max_time,
        const std::shared_ptr<utils::RandomNumberGenerator> &rng,
        const std::shared_ptr<AbstractTask> &task,
        const FactPair &goal,
        std::unordered_set<int> &&blacklisted_variables) override;
public:
    PatternCollectionGeneratorMultipleRandom(
        bool bidirectional, int max_pdb_size, int max_collection_size,
        double pattern_generation_max_time, double total_max_time,
        double stagnation_limit, double blacklist_trigger_percentage,
        bool enable_blacklist_on_stagnation, int random_seed,
        utils::Verbosity verbosity);
};
}

#endif
