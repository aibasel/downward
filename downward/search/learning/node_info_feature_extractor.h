#ifndef NODEINFOFEATUREEXTRACTOR_H_
#define NODEINFOFEATUREEXTRACTOR_H_

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

#endif /* NODEINFOFEATUREEXTRACTOR_H_ */
