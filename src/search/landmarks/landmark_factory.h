#ifndef LANDMARKS_LANDMARK_FACTORY
#define LANDMARKS_LANDMARK_FACTORY

#include "landmark_graph.h"
#include "exploration.h"

class LandmarkFactory {
public:
    LandmarkFactory(LandmarkGraph::Options &options, Exploration *exploration);
    virtual ~LandmarkFactory() {};
    // TODO: compute_lm_graph *must* be called to avoid memory leeks!
    // returns a landmarkgraph created by a factory class.
    // take care to delete the pointer when you don't need it anymore!
    // (method is principally anyways called by every inheriting class)
    LandmarkGraph *compute_lm_graph();
protected:
    LandmarkGraph *lm_graph;
    virtual void generate_landmarks() = 0;
};

#endif
