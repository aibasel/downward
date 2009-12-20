#include "landmarks_feature_extractor.h"

LandmarksFeatureExtractor::LandmarksFeatureExtractor(LandmarksGraph *graph):
	lm_graph(*graph),lm_status_manager(lm_graph) {
}

LandmarksFeatureExtractor::~LandmarksFeatureExtractor() {
}


int LandmarksFeatureExtractor::get_num_features() {
	return lm_graph.number_of_landmarks();
}

int LandmarksFeatureExtractor::get_feature_domain_size(int feature)
{
	assert( (feature >= 0) && (feature < get_num_features()) );
	if ((feature >= 0)&& (feature < get_num_features()))
	    return 3;
	else
	    return -1;
}

void LandmarksFeatureExtractor::extract_features(const void *obj,
		vector<int>& features)
{
    const State &state = *((const State*) obj);
    lm_status_manager.update_lm_status(state);

	const set<LandmarkNode*>& nodes = lm_graph.get_nodes();
	set<LandmarkNode*>::const_iterator it;
	for(it = nodes.begin(); it != nodes.end(); it++) {
		features.push_back( (*it)->status );
	}
}
