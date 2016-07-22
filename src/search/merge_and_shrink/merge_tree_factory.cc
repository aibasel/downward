#include "merge_tree_factory.h"

#include "merge_tree.h"

#include "../options/option_parser.h"
#include "../options/plugin.h"

#include "../utils/rng_options.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeTreeFactory::MergeTreeFactory(const options::Options &options) {
    rng = utils::parse_rng_from_options(options);
}

void MergeTreeFactory::dump_options() const {
    cout << "Merge tree options: " << endl;
    cout << "Type: " << name() << endl;
    dump_tree_specific_options();
}

void MergeTreeFactory::add_options_to_parser(options::OptionParser &parser) {
    utils::add_rng_options(parser);
    vector<string> update_option;
    update_option.push_back("use_first");
    update_option.push_back("use_second");
    update_option.push_back("use_random");
    parser.add_enum_option(
        "update_option",
        update_option,
        "When the merge tree is used within another merge strategy, how"
        "should it be updated when a merge different to a merge from the "
        "tree is performed: choose among use_First, use_second, and "
        "use_random to let the left (right, random) node in the tree survive "
        "and move the other subtree to its position.",
        "use_random");

}

static options::PluginTypePlugin<MergeTreeFactory> _type_plugin(
    "Merge Tree",
    "This page describes merge trees that can be used in merge strategies "
    "of type 'precomputed'.");
}
