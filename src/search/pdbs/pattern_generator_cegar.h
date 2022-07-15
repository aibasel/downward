#ifndef PDBS_PATTERN_GENERATOR_CEGAR_H
#define PDBS_PATTERN_GENERATOR_CEGAR_H

#include "pattern_generator.h"

namespace utils {
class RandomNumberGenerator;
}

namespace pdbs {
class PatternGeneratorCEGAR : public PatternGenerator {
    const int max_pdb_size;
    const int max_time;
    const bool use_wildcard_plans;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    virtual std::string name() const override;
    virtual PatternInformation compute_pattern(
        const std::shared_ptr<AbstractTask> &task) override;
public:
    explicit PatternGeneratorCEGAR(options::Options &opts);
};
}

#endif
