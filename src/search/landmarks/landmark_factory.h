#ifndef LANDMARKS_LANDMARK_FACTORY
#define LANDMARKS_LANDMARK_FACTORY

#include "landmark_graph.h"
#include "exploration.h"

class LandmarkFactory {
public:
    LandmarkFactory(LandmarkGraph::Options &options, Exploration *expl);
    virtual ~LandmarkFactory();
};

#endif
