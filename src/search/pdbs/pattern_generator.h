#ifndef PDBS_PATTERN_GENERATOR_H
#define PDBS_PATTERN_GENERATOR_H

#include "pattern_collection_information.h"
#include "types.h"

#include <memory>

class AbstractTask;

namespace pdbs {
class PatternCollectionGenerator {
public:
    virtual ~PatternCollectionGenerator() = default;

    virtual PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task) = 0;
};

class PatternGenerator {
public:
    virtual ~PatternGenerator() = default;

    virtual Pattern generate(const std::shared_ptr<AbstractTask> &task) = 0;
};
}

#endif
