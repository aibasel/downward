#ifndef G_FEATURE_EXTRACTOR_H_
#define G_FEATURE_EXTRACTOR_H_

#include "node_info_feature_extractor.h"

class GFeatureExtractor : public NodeInfoFeatureExtractor{
protected:
    int max_g;
public:
    GFeatureExtractor(SearchSpace *space);
    virtual ~GFeatureExtractor();

    virtual int get_num_features();
    virtual int get_feature_domain_size(int feature);
    virtual void extract_features(const void *obj, vector<int>& features);
};

#endif /* G_FEATURE_EXTRACTOR_H_ */
