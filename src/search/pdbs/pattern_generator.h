#ifndef PDBS_PATTERN_GENERATOR_H
#define PDBS_PATTERN_GENERATOR_H

#include "pattern_collection_information.h"
#include "pattern_information.h"
#include "types.h"

#include "../component.h"

#include "../utils/logging.h"

#include <memory>
#include <string>

class AbstractTask;

namespace plugins {
class Options;
class Feature;
}

namespace utils {
class RandomNumberGenerator;
}

namespace pdbs {
class PatternCollectionGenerator : public components::TaskSpecificComponent {
    virtual std::string name() const = 0;
    virtual PatternCollectionInformation compute_patterns(
        const std::shared_ptr<AbstractTask> &task) = 0;
protected:
    mutable utils::LogProxy log;
public:
    PatternCollectionGenerator(
        const std::shared_ptr<AbstractTask> &task, utils::Verbosity verbosity);

    PatternCollectionInformation generate(
        const std::shared_ptr<AbstractTask> &task);
};

using TaskIndependentPatternCollectionGenerator =
    components::TaskIndependentComponent<PatternCollectionGenerator>;

class PatternGenerator : public components::TaskSpecificComponent {
    virtual std::string name() const = 0;
    virtual PatternInformation compute_pattern(
        const std::shared_ptr<AbstractTask> &task) = 0;
protected:
    mutable utils::LogProxy log;
public:
    PatternGenerator(
        const std::shared_ptr<AbstractTask> &task, utils::Verbosity verbosity);

    PatternInformation generate(const std::shared_ptr<AbstractTask> &task);
};

using TaskIndependentPatternGenerator =
    components::TaskIndependentComponent<PatternGenerator>;

extern void add_generator_options_to_feature(plugins::Feature &feature);
extern std::tuple<utils::Verbosity> get_generator_arguments_from_options(
    const plugins::Options &opts);
}

#endif
