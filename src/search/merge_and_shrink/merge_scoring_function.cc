#include "merge_scoring_function.h"

#include "../options/plugin.h"

#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeScoringFunction::MergeScoringFunction()
    : initialized(false) {
}

void MergeScoringFunction::dump_options(utils::LogProxy &log) const {
    if (log.is_at_least_normal()) {
        log << "Merge scoring function:" << endl;
        log << "Name: " << name() << endl;
        dump_function_specific_options(log);
    }
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
