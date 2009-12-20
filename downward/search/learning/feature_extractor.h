#ifndef FEATURE_EXTRACTOR_H_
#define FEATURE_EXTRACTOR_H_

/**
 * This class is the base class for feature extractors
 */

#include <vector>
#include "../state.h"
#include "../globals.h"

struct FeatureExtractors {
	FeatureExtractors(bool sv = true, bool ni = false, bool lm = false):
		state_vars(sv),node_info(ni), landmarks(lm) {}
	bool state_vars;
	bool node_info;
	bool landmarks;
};

using namespace std;

#define UNKNOWN_CLASS -1

class FeatureExtractor {
public:
	virtual int get_num_features() = 0;
	virtual int get_feature_domain_size(int feature) = 0;
	virtual void extract_features(const void *obj, vector<int>& features) = 0;
};

#endif /* FEATURE_EXTRACTOR_H_ */
