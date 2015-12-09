#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_MANUAL_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_MANUAL_H

#include "pattern_generator.h"
#include "types.h"

#include <memory>

class Options;


namespace PDBs {
class PatternCollectionGeneratorManual : public PatternCollectionGenerator {
    std::shared_ptr<PatternCollection> patterns;
public:
    explicit PatternCollectionGeneratorManual(const Options &opts);
    virtual ~PatternCollectionGeneratorManual() = default;

    virtual PatternCollectionInformation generate(
        std::shared_ptr<AbstractTask> task) override;
};
}

#endif
