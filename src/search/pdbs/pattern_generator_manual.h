#ifndef PDBS_PATTERN_GENERATOR_MANUAL_H
#define PDBS_PATTERN_GENERATOR_MANUAL_H

#include "pattern_generator.h"
#include "types.h"

namespace options {
class Options;
}

namespace pdbs {
class PatternGeneratorManual : public PatternGenerator {
    Pattern pattern;
public:
    explicit PatternGeneratorManual(const options::Options &opts);
    virtual ~PatternGeneratorManual() = default;

    virtual PatternInformation generate(const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
