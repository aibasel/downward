#ifndef STATEVARFEATUREEXTRACTOR_H_
#define STATEVARFEATUREEXTRACTOR_H_

#include "feature_extractor.h"

class StateVarFeatureExtractor : public FeatureExtractor{
public:
	StateVarFeatureExtractor();
	virtual ~StateVarFeatureExtractor();

	virtual int get_num_features();
	virtual int get_feature_domain_size(int feature);
	virtual void extract_features(const void *obj, vector<int>& features);
};

#endif /* STATEVARFEATUREEXTRACTOR_H_ */
