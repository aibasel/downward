#include "pattern_generator.h"

#include "utils.h"

#include "../plugin.h"

#include "../utils/logging.h"

using namespace std;

namespace pdbs {
PatternCollectionGenerator::PatternCollectionGenerator(const options::Options &opts)
    : verbosity(opts.get<utils::Verbosity>("verbosity")) {
}

PatternCollectionInformation PatternCollectionGenerator::generate(
    const shared_ptr<AbstractTask> &task) {
    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << "Generating patterns using: " << name() << endl;
    }
    utils::Timer timer;
    PatternCollectionInformation pci = compute_patterns(task);
    if (verbosity >= utils::Verbosity::NORMAL) {
        dump_pattern_collection_generation_statistics(
            name(), timer(), pci);
    }
    return pci;
}

PatternGenerator::PatternGenerator(const options::Options &opts)
    : verbosity(opts.get<utils::Verbosity>("verbosity")) {
}

PatternInformation PatternGenerator::generate(
    const shared_ptr<AbstractTask> &task) {
    if (verbosity >= utils::Verbosity::NORMAL) {
        utils::g_log << "Generating pattern using: " << name() << endl;
    }
    utils::Timer timer;
    PatternInformation pattern_info = compute_pattern(task);
    if (verbosity >= utils::Verbosity::NORMAL) {
        dump_pattern_generation_statistics(
            name(),
            timer.stop(),
            pattern_info);
    }
    return pattern_info;
}

void add_generator_options_to_parser(options::OptionParser &parser) {
    utils::add_log_options_to_parser(parser);
}

static PluginTypePlugin<PatternCollectionGenerator> _type_plugin_collection(
    "PatternCollectionGenerator",
    "Factory for pattern collections");

static PluginTypePlugin<PatternGenerator> _type_plugin_single(
    "PatternGenerator",
    "Factory for single patterns");
}
