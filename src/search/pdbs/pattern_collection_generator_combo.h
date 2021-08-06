#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_COMBO_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_COMBO_H

#include "pattern_generator.h"

namespace utils {
class RandomNumberGenerator;
}

namespace pdbs {
/* Take one large pattern and then single-variable patterns for
   all goal variables that are not in the large pattern. */
class PatternCollectionGeneratorCombo : public PatternCollectionGenerator {
    int max_states;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
public:
    explicit PatternCollectionGeneratorCombo(const options::Options &opts);
    virtual ~PatternCollectionGeneratorCombo() = default;

    virtual PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
