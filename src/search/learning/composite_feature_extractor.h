#ifndef LEARNING_COMPOSITE_FEATURE_EXTRACTOR_H
#define LEARNING_COMPOSITE_FEATURE_EXTRACTOR_H

#include "feature_extractor.h"
#include "../global_state.h"

#include <vector>

class CompositeFeatureExtractor : public FeatureExtractor {
protected:
    vector<FeatureExtractor *> feature_extractors;
    int num_features;
public:
    CompositeFeatureExtractor();
    virtual ~CompositeFeatureExtractor();

    void add_feature_extractor(FeatureExtractor *fe);

    virtual int get_num_features() {return num_features; }
    virtual int get_feature_domain_size(int feature);
    virtual void extract_features(const void *obj, vector<int> &features);
};

#endif
