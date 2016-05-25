#ifndef PDBS_PATTERN_GENERATOR_H
#define PDBS_PATTERN_GENERATOR_H

#include "pattern_collection_information.h"
#include "types.h"

#include <memory>

class AbstractTask;

namespace pdbs {
class PatternCollectionGenerator {
public:
    virtual PatternCollectionInformation generate(std::shared_ptr<AbstractTask> task) = 0;
};

class PatternGenerator {
public:
    virtual Pattern generate(std::shared_ptr<AbstractTask> task) = 0;
};
}

#endif
