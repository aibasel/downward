#ifndef COMPOSITEFEATUREEXTRACTOR_H_
#define COMPOSITEFEATUREEXTRACTOR_H_

#include <vector>
#include "feature_extractor.h"
#include "../state.h"

class CompositeFeatureExtractor : public FeatureExtractor{
protected:
	vector<FeatureExtractor*> feature_extractors;
	int num_features;
public:
	CompositeFeatureExtractor();
	virtual ~CompositeFeatureExtractor();

	void add_feature_extractor(FeatureExtractor* fe);

	virtual int get_num_features() {return num_features;}
	virtual int get_feature_domain_size(int feature);
	virtual void extract_features(const void *obj, vector<int>& features);
};

#endif /* COMPOSITEFEATUREEXTRACTOR_H_ */
