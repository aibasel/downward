#ifndef LANDMARKS_LANDMARK_FACTORY_ACYCLIC_H
#define LANDMARKS_LANDMARK_FACTORY_ACYCLIC_H

#include "landmark_factory.h"

namespace options {
class OptionParser;
class Options;
}

namespace landmarks {
class LandmarkFactoryAcyclic : public LandmarkFactory {
    std::shared_ptr<LandmarkFactory> lm_factory;

    virtual void generate_landmarks(const std::shared_ptr<AbstractTask> &task) override;

    int loop_acyclic_graph(LandmarkNode &lmn,
                           std::unordered_set<LandmarkNode *> &acyclic_node_set);
    bool remove_first_weakest_cycle_edge(LandmarkNode *cur,
                                         std::list<std::pair<LandmarkNode *, EdgeType>> &path,
                                         std::list<std::pair<LandmarkNode *, EdgeType>>::iterator it);
public:
    explicit LandmarkFactoryAcyclic(const options::Options &opts);

    virtual bool supports_conditional_effects() const override;
};
}
#endif
