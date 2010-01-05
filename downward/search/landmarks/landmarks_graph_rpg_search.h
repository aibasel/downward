#ifndef LANDMARKSGRAPHSEARCH_H
#define LANDMARKSGRAPHSEARCH_H

#include "landmarks_graph.h"

typedef LandmarkNode* LMOpenListEntry;

class LandmarksGraphRpgSearch: public LandmarksGraph {
    bool uniform_sampling;
    int max_depth;
    int num_tries;
    void generate_landmarks();
    void landmark_search(LandmarkNode *node, int depth);
    int choose_random(vector<int> &evals);
public:
    LandmarksGraphRpgSearch(Exploration* exploration);
    virtual ~LandmarksGraphRpgSearch();
};

#endif
