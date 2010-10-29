#include "composite_feature_extractor.h"
#include <cassert>

CompositeFeatureExtractor::CompositeFeatureExtractor() {
    num_features = 0;
}

CompositeFeatureExtractor::~CompositeFeatureExtractor() {
}

void CompositeFeatureExtractor::add_feature_extractor(FeatureExtractor *fe) {
    feature_extractors.push_back(fe);
    num_features = num_features + fe->get_num_features();
}

int CompositeFeatureExtractor::get_feature_domain_size(int feature) {
    assert((feature >= 0) && (feature < num_features));

    int fe = 0;
    while (feature >= 0) {
        feature = feature - feature_extractors[fe]->get_num_features();
        if (feature < 0) {
            feature = feature + feature_extractors[fe]->get_num_features();
            return feature_extractors[fe]->get_feature_domain_size(feature);
        }
        fe++;
    }
    return 0;
}

void CompositeFeatureExtractor::extract_features(const void *obj,
                                                 vector<int> &features) {
    for (int i = 0; i < feature_extractors.size(); i++) {
        feature_extractors[i]->extract_features(obj, features);
    }
}
