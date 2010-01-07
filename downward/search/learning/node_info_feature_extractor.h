#ifndef LEARNING_NODE_INFO_FEATURE_EXTRACTOR_H
#define LEARNING_NODE_INFO_FEATURE_EXTRACTOR_H

#include "feature_extractor.h"
#include "../search_space.h"

/**
 * Adds the following features:
 * 1. g
 */
class NodeInfoFeatureExtractor: public FeatureExtractor {
protected:
	SearchSpace *search_space;
public:
	NodeInfoFeatureExtractor(SearchSpace *space);
	virtual ~NodeInfoFeatureExtractor();

	void change_search_space(SearchSpace *space) {search_space = space;}
};

#endif
