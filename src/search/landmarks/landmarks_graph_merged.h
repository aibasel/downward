#ifndef LANDMARKS_LANDMARKS_GRAPH_MERGED_H
#define LANDMARKS_LANDMARKS_GRAPH_MERGED_H

#include "landmarks_graph.h"
#include <vector>


class LandmarksGraphMerged : public LandmarksGraph {
    std::vector<LandmarksGraph *> lm_graphs;
    void generate_landmarks();
    LandmarkNode *get_matching_landmark(const LandmarkNode &lm) const;
public:
    LandmarksGraphMerged(LandmarkGraphOptions &options,
                         Exploration *exploration,
                         const std::vector<LandmarksGraph *> &lm_graphs_);
    virtual ~LandmarksGraphMerged();
    static LandmarksGraph *create(const std::vector<std::string> &config, int start,
                                  int &end, bool dry_run);
};

#endif
