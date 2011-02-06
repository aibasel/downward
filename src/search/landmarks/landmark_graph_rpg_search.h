#ifndef LANDMARKS_LANDMARK_GRAPH_RPG_SEARCH_H
#define LANDMARKS_LANDMARK_GRAPH_RPG_SEARCH_H

#include "landmark_factory.h"
#include "landmark_graph.h"

typedef LandmarkNode *LMOpenListEntry;

class LandmarkGraphRpgSearch : public LandmarkFactory {
    bool uniform_sampling;
    int max_depth;
    int num_tries;
    void generate_landmarks();
    void landmark_search(LandmarkNode *node, int depth);
    int choose_random(vector<int> &evals);
public:
    LandmarkGraphRpgSearch(LandmarkGraph::Options &options,
                            Exploration *exploration,
                            bool uniform_sampling_, int max_depth_, int num_tries_);
    virtual ~LandmarkGraphRpgSearch();
    static LandmarkGraph *create(const std::vector<std::string> &config, int start,
                                  int &end, bool dry_run);
};

#endif
