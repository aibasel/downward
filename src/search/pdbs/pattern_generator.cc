#include "pattern_generator.h"

#include "../plugin.h"

#include "../utils/logging.h"

namespace pdbs {
PatternCollectionGenerator::PatternCollectionGenerator(const options::Options &opts)
    : verbosity(opts.get<utils::Verbosity>("verbosity")) {
}

PatternGenerator::PatternGenerator(const options::Options &opts)
    : verbosity(opts.get<utils::Verbosity>("verbosity")) {
}

void add_generator_options_to_parser(options::OptionParser &parser) {
    utils::add_verbosity_option_to_parser(parser);
}

static PluginTypePlugin<PatternCollectionGenerator> _type_plugin_collection(
    "PatternCollectionGenerator",
    "Factory for pattern collections");

static PluginTypePlugin<PatternGenerator> _type_plugin_single(
    "PatternGenerator",
    "Factory for single patterns");
}
