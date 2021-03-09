#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_SINGLE_CEGAR_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_SINGLE_CEGAR_H

#include "pattern_generator.h"

#include "cegar.h"
#include "types.h"

#include "../utils/rng.h"

namespace utils {
enum class Verbosity;
}

namespace pdbs {
class PatternCollectionGeneratorSingleCegar : public PatternCollectionGenerator {
    const int max_pdb_size;
    const int max_collection_size;
    const bool wildcard_plans;
    const double max_time;
    const utils::Verbosity verbosity;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
public:
    explicit PatternCollectionGeneratorSingleCegar(const options::Options &opts);
    virtual ~PatternCollectionGeneratorSingleCegar() = default;

    virtual PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
