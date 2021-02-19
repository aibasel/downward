#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_CEGAR_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_CEGAR_H

#include "pattern_generator.h"

namespace options {
class Options;
}

namespace utils {
enum class Verbosity;
}

namespace pdbs {
class PatternCollectionGeneratorMultipleCegar : public PatternCollectionGenerator {
    const int cegar_max_refinements;
    const int cegar_max_pdb_size;
    const int cegar_max_collection_size; // Possibly overwritten by total_collection_max_size
    const bool cegar_wildcard_plans;
    const double cegar_max_time; // Possibly overwritten by remaining total_time_limit

    const utils::Verbosity verbosity;
    const int initial_random_seed;
    const int total_collection_max_size;
    const double stagnation_limit;
    const double blacklist_trigger_time;
    const bool blacklist_on_stagnation;
    const double total_time_limit;
public:
    explicit PatternCollectionGeneratorMultipleCegar(options::Options &opts);
    virtual ~PatternCollectionGeneratorMultipleCegar() = default;

    virtual PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
