#ifndef LEARNING_FEATURE_EXTRACTOR_H
#define LEARNING_FEATURE_EXTRACTOR_H

/**
 * This class is the base class for feature extractors
 */

#include <vector>

using namespace std;

#define UNKNOWN_CLASS -1

class FeatureExtractor {
public:
    virtual int get_num_features() = 0;
    virtual int get_feature_domain_size(int feature) = 0;
    virtual void extract_features(const void *obj, vector<int> &features) = 0;
};


struct FeatureExtractorFactory {
    bool state_vars;

    FeatureExtractorFactory(bool sv = true) : state_vars(sv) {}
    FeatureExtractor *create();
};


#endif
