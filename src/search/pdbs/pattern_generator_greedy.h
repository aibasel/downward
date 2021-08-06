#ifndef PDBS_PATTERN_GENERATOR_GREEDY_H
#define PDBS_PATTERN_GENERATOR_GREEDY_H

#include "pattern_generator.h"

namespace options {
class Options;
}

namespace utils {
class RandomNumberGenerator;
}

namespace pdbs {
class PatternGeneratorGreedy : public PatternGenerator {
    int max_states;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
public:
    explicit PatternGeneratorGreedy(const options::Options &opts);
    explicit PatternGeneratorGreedy(
        int max_states, const std::shared_ptr<utils::RandomNumberGenerator> &rng);
    virtual ~PatternGeneratorGreedy() = default;

    virtual PatternInformation generate(const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
