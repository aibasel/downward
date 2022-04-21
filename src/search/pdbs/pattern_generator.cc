#include "pattern_generator.h"

#include "utils.h"

#include "../plugin.h"

using namespace std;

namespace pdbs {
PatternCollectionGenerator::PatternCollectionGenerator(const options::Options &opts)
    : log(utils::get_log_from_options(opts)) {
}

PatternCollectionInformation PatternCollectionGenerator::generate(
    const shared_ptr<AbstractTask> &task) {
    if (log.is_at_least_normal()) {
        log << "Generating patterns using: " << name() << endl;
    }
    utils::Timer timer;
    PatternCollectionInformation pci = compute_patterns(task);
    dump_pattern_collection_generation_statistics(
        name(), timer(), pci, log);
    return pci;
}

PatternGenerator::PatternGenerator(const options::Options &opts)
    : log(utils::get_log_from_options(opts)) {
}

PatternInformation PatternGenerator::generate(
    const shared_ptr<AbstractTask> &task) {
    if (log.is_at_least_normal()) {
        log << "Generating pattern using: " << name() << endl;
    }
    utils::Timer timer;
    PatternInformation pattern_info = compute_pattern(task);
    dump_pattern_generation_statistics(
        name(),
        timer.stop(),
        pattern_info,
        log);
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
