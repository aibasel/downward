#ifndef LANDMARKS_LANDMARK_GRAPH_MERGED_H
#define LANDMARKS_LANDMARK_GRAPH_MERGED_H

#include "landmark_factory.h"
#include "landmark_graph.h"

#include <vector>

namespace landmarks {
class LandmarkGraphMerged : public LandmarkFactory {
    std::vector<std::unique_ptr<LandmarkGraph>> lm_graphs;
    std::vector<LandmarkFactory *> lm_factories;

    void generate_landmarks(Exploration &exploration);
    LandmarkNode *get_matching_landmark(const LandmarkNode &lm) const;
public:
    LandmarkGraphMerged(const options::Options &opts);
    virtual ~LandmarkGraphMerged();
};
}

#endif
