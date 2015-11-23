#ifndef PDBS_PATTERN_GENERATION_COMBO_H
#define PDBS_PATTERN_GENERATION_COMBO_H

#include "pattern_generator.h"

class PatternGenerationCombo : public PatternCollectionGenerator {
    int max_states;
public:
    explicit PatternGenerationCombo(const Options &opts);
    virtual ~PatternGenerationCombo() = default;

    virtual PatternCollection generate(std::shared_ptr<AbstractTask> task) override;
};


#endif
