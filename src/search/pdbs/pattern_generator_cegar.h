#ifndef PDBS_PATTERN_GENERATOR_CEGAR_H
#define PDBS_PATTERN_GENERATOR_CEGAR_H

#include "pattern_generator.h"

namespace utils {
class RandomNumberGenerator;
}

namespace pdbs {
class PatternGeneratorCEGAR : public PatternGenerator {
    const int max_pdb_size;
    const double max_time;
    const bool use_wildcard_plans;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    virtual std::string name() const override;
    virtual PatternInformation compute_pattern(
        const std::shared_ptr<AbstractTask> &task) override;
public:
    PatternGeneratorCEGAR(
        int max_pdb_size, double max_time, bool use_wildcard_plans,
        int random_seed, utils::Verbosity verbosity);
};
}

#endif
