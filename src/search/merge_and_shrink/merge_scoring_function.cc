#include "merge_scoring_function.h"

#include "../options/plugin.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
void MergeScoringFunction::dump_options() const {
    cout << "Merge scoring function:" << endl;
    cout << "Name: " << name() << endl;
    dump_specific_options();
}

static options::PluginTypePlugin<MergeScoringFunction> _type_plugin(
    "Merge Scoring Function",
    "This page describes merge scoring functions that can be used in score "
    "based merge selectors.");
}
