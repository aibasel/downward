#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_DISJOINT_CEGAR_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_DISJOINT_CEGAR_H

#include "pattern_generator.h"

namespace utils {
class RandomNumberGenerator;
}

namespace pdbs {
/*
  This pattern collection generator uses the CEGAR algorithm to compute a
  disjoint pattern collection for the given task. See cegar.h for more details.
*/
class PatternCollectionGeneratorDisjointCegar : public PatternCollectionGenerator {
    const int max_pdb_size;
    const int max_collection_size;
    const double max_time;
    const bool use_wildcard_plans;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    virtual std::string name() const override;
    virtual PatternCollectionInformation compute_patterns(
        const std::shared_ptr<AbstractTask> &task) override;
public:
    PatternCollectionGeneratorDisjointCegar(
        int max_pdb_size, int max_collection_size, double max_time,
        bool use_wildcard_plans, int random_seed,
        utils::Verbosity verbosity);
};
}

#endif
