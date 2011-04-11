#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_SEARCH_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_SEARCH_H

#include "landmark_factory.h"
#include "landmark_graph.h"

typedef LandmarkNode *LMOpenListEntry;

class LandmarkFactoryRpgSearch : public LandmarkFactory {
    bool uniform_sampling;
    int max_depth;
    int num_tries;
    void generate_landmarks();
    void landmark_search(LandmarkNode *node, int depth);
    int choose_random(vector<int> &evals);
    int relaxed_plan_length_without(LandmarkNode *lm);
public:
    LandmarkFactoryRpgSearch(const Options &opts);
    virtual ~LandmarkFactoryRpgSearch();
};

#endif
