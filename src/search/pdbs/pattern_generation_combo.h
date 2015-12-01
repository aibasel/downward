#ifndef PDBS_PATTERN_GENERATION_COMBO_H
#define PDBS_PATTERN_GENERATION_COMBO_H

#include "pattern_generator.h"

/* Take one large pattern and then single-variable patterns for
   all goal variables that are not in the large pattern. */
class PatternCollectionGeneratorCombo : public PatternCollectionGenerator {
    int max_states;
public:
    explicit PatternCollectionGeneratorCombo(const Options &opts);
    virtual ~PatternCollectionGeneratorCombo() = default;

    virtual PatternCollectionInformation generate(
        std::shared_ptr<AbstractTask> task) override;
};


#endif
