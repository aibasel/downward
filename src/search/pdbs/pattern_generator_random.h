#ifndef PDBS_PATTERN_GENERATOR_RANDOM_H
#define PDBS_PATTERN_GENERATOR_RANDOM_H

#include "pattern_generator.h"

namespace utils {
class RandomNumberGenerator;
}

namespace pdbs {
class PatternGeneratorRandom : public PatternGenerator {
    const int max_pdb_size;
    const double max_time;
    const bool bidirectional;
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    virtual std::string name() const override;
    virtual PatternInformation compute_pattern(
        const std::shared_ptr<AbstractTask> &task) override;
public:
    explicit PatternGeneratorRandom(const plugins::Options &opts);
};
}

#endif
