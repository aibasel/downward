#ifndef PDBS_PATTERN_GENERATOR_MANUAL_H
#define PDBS_PATTERN_GENERATOR_MANUAL_H

#include "pattern_generator.h"
#include "types.h"

class Options;


namespace PDBs {
class PatternGeneratorManual : public PatternGenerator {
    Pattern pattern;
public:
    explicit PatternGeneratorManual(const Options &opts);
    virtual ~PatternGeneratorManual() = default;

    virtual Pattern generate(std::shared_ptr<AbstractTask> task) override;
};
}

#endif
