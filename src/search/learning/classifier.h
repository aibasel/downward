#ifndef LEARNING_CLASSIFIER_H
#define LEARNING_CLASSIFIER_H

#include "feature_extractor.h"
#include "../global_state.h"

enum classifier_t {NB = 0, AODE = 1};

class Classifier {
protected:
    FeatureExtractor *feature_extractor;
public:
    Classifier();
    virtual ~Classifier();
    virtual void buildClassifier(int num_classes) = 0;
    virtual void addExample(const void *obj, int tag) = 0;
    virtual bool distributionForInstance(const void *obj, double *dist) = 0;

    FeatureExtractor *get_feature_extractor() const {return feature_extractor; }
    void set_feature_extractor(FeatureExtractor *fe) {feature_extractor = fe; }
};

#endif
