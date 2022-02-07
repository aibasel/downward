#ifndef PDBS_PATTERN_GENERATOR_H
#define PDBS_PATTERN_GENERATOR_H

#include "pattern_collection_information.h"
#include "pattern_information.h"
#include "types.h"

#include <memory>

class AbstractTask;

namespace options {
class OptionParser;
class Options;
}

namespace utils {
class RandomNumberGenerator;
enum class Verbosity;
}

namespace pdbs {
class PatternCollectionGenerator {
protected:
    const utils::Verbosity verbosity;
public:
    explicit PatternCollectionGenerator(const options::Options &opts);
    virtual ~PatternCollectionGenerator() = default;

    virtual PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task) = 0;
};

class PatternGenerator {
protected:
    const utils::Verbosity verbosity;
public:
    explicit PatternGenerator(const options::Options &opts);
    virtual ~PatternGenerator() = default;

    virtual PatternInformation generate(const std::shared_ptr<AbstractTask> &task) = 0;
};

extern void add_generator_options_to_parser(options::OptionParser &parser);
}

#endif
