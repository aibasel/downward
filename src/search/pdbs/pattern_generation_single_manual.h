#ifndef PDBS_PATTERN_GENERATION_SINGLE_MANUAL_H
#define PDBS_PATTERN_GENERATION_SINGLE_MANUAL_H

#include "pattern_generator.h"

class PatternGenerationSingleManual : public PatternGenerator {
    Pattern pattern;
public:
    explicit PatternGenerationSingleManual(const Options &opts);
    virtual ~PatternGenerationSingleManual() = default;

    virtual Pattern generate(std::shared_ptr<AbstractTask> task) override;
};


#endif
