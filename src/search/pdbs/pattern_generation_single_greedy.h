#ifndef PDBS_PATTERN_GENERATION_SINGLE_GREEDY_H
#define PDBS_PATTERN_GENERATION_SINGLE_GREEDY_H

#include "pattern_generator.h"

class PatternGenerationSingleGreedy : public PatternGenerator {
    int max_states;
public:
    explicit PatternGenerationSingleGreedy(const Options &opts);
    explicit PatternGenerationSingleGreedy(int max_states);
    virtual ~PatternGenerationSingleGreedy() = default;

    virtual Pattern generate(std::shared_ptr<AbstractTask> task) override;
};

#endif
