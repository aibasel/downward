#ifndef LANDMARKS_LANDMARK_GRAPH_MERGED_H
#define LANDMARKS_LANDMARK_GRAPH_MERGED_H

#include "landmark_factory.h"
#include "landmark_graph.h"
#include <vector>

class LandmarkGraphMerged : public LandmarkFactory {
    std::vector<LandmarkGraph *> lm_graphs;
    void generate_landmarks();
    LandmarkNode *get_matching_landmark(const LandmarkNode &lm) const;
public:
    LandmarkGraphMerged(const Options &opts);
    virtual ~LandmarkGraphMerged();
};

#endif
