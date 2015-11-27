#ifndef PDBS_PATTERN_GENERATION_COMBO_H
#define PDBS_PATTERN_GENERATION_COMBO_H

#include "pattern_generator.h"

/* Take one large pattern and then single-variable patterns for
   all goal variables that are not in the large pattern. */
class PatternGenerationCombo : public PatternCollectionGenerator {
    int max_states;
public:
    explicit PatternGenerationCombo(const Options &opts);
    virtual ~PatternGenerationCombo() = default;

    virtual PatternCollection generate(
        std::shared_ptr<AbstractTask> task) override;
};


#endif
