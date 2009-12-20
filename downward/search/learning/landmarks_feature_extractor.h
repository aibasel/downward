#ifndef LANDMARKSFEATUREEXTRACTOR_H_
#define LANDMARKSFEATUREEXTRACTOR_H_

#include "feature_extractor.h"
#include "../landmarks/landmarks_graph.h"
#include "../landmarks/landmark_status_manager.h"

class LandmarksFeatureExtractor: public FeatureExtractor {
protected:
	LandmarksGraph &lm_graph;
	LandmarkStatusManager lm_status_manager;

public:
	LandmarksFeatureExtractor(LandmarksGraph *graph);
	virtual ~LandmarksFeatureExtractor();

	virtual int get_num_features();
	virtual int get_feature_domain_size(int feature);
	virtual void extract_features(const void *obj, vector<int>& features);
};

#endif /* LANDMARKSFEATUREEXTRACTOR_H_ */
