#ifndef LANDMARKS_LANDMARK_GRAPH_MERGED_H
#define LANDMARKS_LANDMARK_GRAPH_MERGED_H

#include "landmark_factory.h"

#include <vector>

namespace landmarks {
class LandmarkGraph;
class LandmarkNode;

class LandmarkGraphMerged : public LandmarkFactory {
    std::vector<std::shared_ptr<LandmarkGraph>> lm_graphs;
    std::vector<LandmarkFactory *> lm_factories;

    virtual void generate_landmarks(const std::shared_ptr<AbstractTask> &task, Exploration &exploration) override;
    LandmarkNode *get_matching_landmark(const LandmarkNode &lm) const;
public:
    explicit LandmarkGraphMerged(const options::Options &opts);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
