#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_CEGAR_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_CEGAR_H

#include "pattern_generator.h"

namespace options {
class Options;
}

namespace utils {
class RandomNumberGenerator;
enum class Verbosity;
}

namespace pdbs {
class PatternCollectionGeneratorMultipleCegar : public PatternCollectionGenerator {
    // Options for the CEGAR algorithm.
    const int max_refinements;
    const int max_pdb_size;
    const int max_collection_size;
    const bool wildcard_plans;
    const double cegar_max_time;

    // Options for this generator.
    const utils::Verbosity verbosity;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    // We store the random seed for creating different RNG objects for CEGAR.
    const int random_seed;
    const double stagnation_limit;
    const double blacklist_trigger_percentage;
    const bool blacklist_on_stagnation;
    const double total_max_time;
public:
    explicit PatternCollectionGeneratorMultipleCegar(options::Options &opts);
    virtual ~PatternCollectionGeneratorMultipleCegar() = default;

    virtual PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
