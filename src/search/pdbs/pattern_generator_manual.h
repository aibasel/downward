#ifndef PDBS_PATTERN_GENERATOR_MANUAL_H
#define PDBS_PATTERN_GENERATOR_MANUAL_H

#include "pattern_generator.h"
#include "types.h"

namespace pdbs {
class PatternGeneratorManual : public PatternGenerator {
    Pattern pattern;

    virtual std::string name() const override;
    virtual PatternInformation compute_pattern(
        const std::shared_ptr<AbstractTask> &task) override;
public:
    explicit PatternGeneratorManual(const options::Options &opts);
    virtual ~PatternGeneratorManual() = default;
};
}

#endif
