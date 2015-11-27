#ifndef PDBS_PATTERN_GENERATION_MANUAL_H
#define PDBS_PATTERN_GENERATION_MANUAL_H

#include "pattern_generator.h"
#include "types.h"

#include <memory>

class Options;

class PatternGenerationManual : public PatternCollectionGenerator {
    std::shared_ptr<Patterns> patterns;
public:
    explicit PatternGenerationManual(const Options &opts);
    virtual ~PatternGenerationManual() = default;

    virtual PatternCollection generate(
        std::shared_ptr<AbstractTask> task) override;
};

#endif
