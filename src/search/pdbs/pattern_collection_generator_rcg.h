#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_RCG_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_RCG_H

#include "pattern_generator.h"

#include <set>

namespace options {
class Options;
}

namespace pdbs {
class PatternCollectionGeneratorRCG : public PatternCollectionGenerator {
    const int single_generator_max_pdb_size;
    const int initial_random_seed;
    const int total_collection_max_size;
    const double stagnation_limit;
    const double total_time_limit;
    const bool disable_output;
    const bool bidirectional;
    const int max_num_iterations;
public:
    explicit PatternCollectionGeneratorRCG(options::Options &opts);
    virtual ~PatternCollectionGeneratorRCG() = default;

    virtual PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
