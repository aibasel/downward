#include "merge_scoring_function.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeScoringFunction::MergeScoringFunction(const plugins::Options &options)
    : use_caching(options.get<bool>("use_caching")),
      initialized(false) {
}

void MergeScoringFunction::dump_options(utils::LogProxy &log) const {
    if (log.is_at_least_normal()) {
        log << "Merge scoring function:" << endl;
        log << "Name: " << name() << endl;
        dump_function_specific_options(log);
    }
}

void add_merge_scoring_function_options_to_feature(plugins::Feature &feature) {
    feature.add_option<bool>(
        "use_caching",
        "Cache scores for merge candidates. Currently only supported by the "
        "MIASM scoring function.",
        "false");
}

static class MergeScoringFunctionCategoryPlugin : public plugins::TypedCategoryPlugin<MergeScoringFunction> {
public:
    MergeScoringFunctionCategoryPlugin() : TypedCategoryPlugin("MergeScoringFunction") {
        document_synopsis(
            "This page describes various merge scoring functions. A scoring function, "
            "given a list of merge candidates and a factored transition system, "
            "computes a score for each candidate based on this information and "
            "potentially some chosen options. Minimal scores are considered best. "
            "Scoring functions are currently only used within the score based "
            "filtering merge selector.");
    }
}
_category_plugin;
}
