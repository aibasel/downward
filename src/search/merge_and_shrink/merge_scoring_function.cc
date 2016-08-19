#include "merge_scoring_function.h"

#include "../options/plugin.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeScoringFunction::MergeScoringFunction()
    : initialized(false) {
}

void MergeScoringFunction::dump_options() const {
    cout << "Merge scoring function:" << endl;
    cout << "Name: " << name() << endl;
    dump_function_specific_options();
}

static options::PluginTypePlugin<MergeScoringFunction> _type_plugin(
    "MergeScoringFunction",
    "This page describes various merge scoring functions. A scoring function, "
    "given a list of merge candidates and a factored transition system, "
    "computes a score for each candidate based on this information and "
    "potentially some chosen options. Minimal scores are considered best. "
    "Scoring functions are currently only used within the score based "
    "filtering merge selector.");
}
