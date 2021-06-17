#ifndef PDBS_PATTERN_GENERATOR_RCG_H
#define PDBS_PATTERN_GENERATOR_RCG_H

#include "pattern_generator.h"

namespace options {
class Options;
}

namespace utils {
class RandomNumberGenerator;
enum class Verbosity;
}

namespace pdbs {
class PatternGeneratorRCG : public PatternGenerator {
    const int max_pdb_size;
    const int max_time;
    const bool bidirectional;
    const utils::Verbosity verbosity;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
public:
    explicit PatternGeneratorRCG(options::Options &opts);
    virtual ~PatternGeneratorRCG() = default;

    virtual PatternInformation generate(
        const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
