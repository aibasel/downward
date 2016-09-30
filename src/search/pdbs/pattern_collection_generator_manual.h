#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_MANUAL_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_MANUAL_H

#include "pattern_generator.h"
#include "types.h"

#include <memory>

namespace options {
class Options;
}

namespace pdbs {
class PatternCollectionGeneratorManual : public PatternCollectionGenerator {
    std::shared_ptr<PatternCollection> patterns;
public:
    explicit PatternCollectionGeneratorManual(const options::Options &opts);
    virtual ~PatternCollectionGeneratorManual() = default;

    virtual PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
