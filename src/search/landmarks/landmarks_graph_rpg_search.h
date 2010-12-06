#ifndef LANDMARKS_LANDMARKS_GRAPH_RPG_SEARCH_H
#define LANDMARKS_LANDMARKS_GRAPH_RPG_SEARCH_H

#include "landmarks_graph.h"

typedef LandmarkNode *LMOpenListEntry;

class LandmarksGraphRpgSearch : public LandmarksGraph {
    bool uniform_sampling;
    int max_depth;
    int num_tries;
    void generate_landmarks();
    void landmark_search(LandmarkNode *node, int depth);
    int choose_random(vector<int> &evals);
public:
    LandmarksGraphRpgSearch(LandmarkGraphOptions &options,
                            Exploration *exploration,
                            bool uniform_sampling_, int max_depth_, int num_tries_);
    virtual ~LandmarksGraphRpgSearch();
    static LandmarksGraph *create(const std::vector<std::string> &config, int start,
                                  int &end, bool dry_run);
};

#endif
