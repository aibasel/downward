#ifndef PDBS_PATTERN_COLLECTION_GENERATOR_COMBO_H
#define PDBS_PATTERN_COLLECTION_GENERATOR_COMBO_H

#include "pattern_generator.h"

#include "../plugins/options.h"

namespace pdbs {
/* Take one large pattern and then single-variable patterns for
   all goal variables that are not in the large pattern. */
class PatternCollectionGeneratorCombo : public PatternCollectionGenerator {
    plugins::Options opts;

    virtual std::string name() const override;
    virtual PatternCollectionInformation compute_patterns(
        const std::shared_ptr<AbstractTask> &task) override;
public:
    explicit PatternCollectionGeneratorCombo(const plugins::Options &opts);
    virtual ~PatternCollectionGeneratorCombo() = default;
};
}

#endif
