#include "merge_tree_factory.h"

#include "../options/plugin.h"

#include "../utils/rng_options.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeTreeFactory::MergeTreeFactory(const options::Options &options) {
    rng = utils::parse_rng_from_options(options);
}

void MergeTreeFactory::add_options_to_parser(options::OptionParser &parser) {
    utils::add_rng_options(parser);
}

static options::PluginTypePlugin<MergeTreeFactory> _type_plugin(
    "Merge Tree",
    "This page describes merge trees that can be used in merge strategies "
    "of type 'precomputed'.");
}
