#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_MANUAL_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_MANUAL_H

#include "pattern_generator.h"
#include "types.h"

#include <memory>

namespace pdbs {
class PatternCollectionGeneratorManual : public PatternCollectionGenerator {
    std::shared_ptr<PatternCollection> patterns;

    virtual std::string name() const override;
    virtual PatternCollectionInformation compute_patterns(
        const std::shared_ptr<AbstractTask> &task) override;
public:
    explicit PatternCollectionGeneratorManual(const plugins::Options &opts);
    virtual ~PatternCollectionGeneratorManual() = default;
};
}

#endif
