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
    LandmarkGraphMerged(LandmarkGraph::Options &options,
                        Exploration *exploration,
                        const std::vector<LandmarkGraph *> &lm_graphs_);
    virtual ~LandmarkGraphMerged();
    static LandmarkGraph *create(const std::vector<std::string> &config, int start,
                                 int &end, bool dry_run);
};

#endif
