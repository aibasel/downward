#include "merge_selector.h"

#include "../options/plugin.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
void MergeSelector::dump_options() const {
    cout << "Merge selector options:" << endl;
    cout << "Name: " << name() << endl;
    dump_specific_options();
}

static options::PluginTypePlugin<MergeSelector> _type_plugin(
    "Merge Selector",
    "This page describes merge selectors that can be used in merge strategies "
    "of type 'stateless'.");
}
