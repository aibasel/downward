#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_SINGLE_CEGAR_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_SINGLE_CEGAR_H

#include "pattern_generator.h"

#include "types.h"

#include "../utils/rng.h"

namespace utils {
enum class Verbosity;
}

namespace pdbs {
enum class InitialCollectionType;

class PatternCollectionGeneratorSingleCegar : public PatternCollectionGenerator {
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    const int max_refinements;
    const int max_pdb_size;
    const int max_collection_size;
    const bool wildcard_plans; // this is passed to AbstractSolutionData constructors, to set what kind of plan should be generated
    const bool ignore_goal_violations; // set this to true if you want to generate only one pattern
    const int global_blacklist_size;
    const InitialCollectionType initial;
    const int given_goal;
    const utils::Verbosity verbosity;
    const double max_time;
    const std::string token = "CEGAR_PDBs: ";
public:
    explicit PatternCollectionGeneratorSingleCegar(const options::Options &opts);
    virtual ~PatternCollectionGeneratorSingleCegar();

    PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
