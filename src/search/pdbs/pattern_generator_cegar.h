#ifndef PDBS_PATTERN_GENERATOR_CEGAR_H
#define PDBS_PATTERN_GENERATOR_CEGAR_H

#include "pattern_generator.h"

namespace options {
class Options;
}

namespace utils {
class RandomNumberGenerator;
enum class Verbosity;
}

namespace pdbs {
class PatternGeneratorCEGAR : public PatternGenerator {
    const int max_pdb_size;
    const int max_time;
    const bool use_wildcard_plans;
    const utils::Verbosity verbosity;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
public:
    explicit PatternGeneratorCEGAR(options::Options &opts);

    virtual PatternInformation generate(
        const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
