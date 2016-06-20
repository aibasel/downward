#include "merge_tree_factory.h"

#include "../options/plugin.h"

using namespace std;

namespace merge_and_shrink {
static options::PluginTypePlugin<MergeTreeFactory> _type_plugin(
    "Merge Tree",
    "This page describes merge trees that can be used in merge strategies "
    "of type 'precomputed'.");
}