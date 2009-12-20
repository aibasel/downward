#include "node_info_feature_extractor.h"
#include <cassert>
#include "../search_space.h"

NodeInfoFeatureExtractor::NodeInfoFeatureExtractor(SearchSpace *space):
	search_space(space) {
}

NodeInfoFeatureExtractor::~NodeInfoFeatureExtractor() {
}
