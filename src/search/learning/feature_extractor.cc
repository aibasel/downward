#include "feature_extractor.h"
#include "state_vars_feature_extractor.h"
#include <cassert>

FeatureExtractor *FeatureExtractorFactory::create() {
    assert(state_vars);
    FeatureExtractor *fe = new StateVarFeatureExtractor;
    return fe;
}
