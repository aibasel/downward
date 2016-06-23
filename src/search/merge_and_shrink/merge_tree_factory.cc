#include "merge_tree_factory.h"

#include "../options/plugin.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeTreeFactory::MergeTreeFactory(const options::Options &options)
    : options(options) {
    if (!options.contains("random_seed")) {
        cerr << "Options for merge tree factories must include a random "
                "seed option!" << endl;
    }
}

static options::PluginTypePlugin<MergeTreeFactory> _type_plugin(
    "Merge Tree",
    "This page describes merge trees that can be used in merge strategies "
    "of type 'precomputed'.");
}
