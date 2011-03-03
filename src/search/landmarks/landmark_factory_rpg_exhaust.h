#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H

#include "landmark_factory.h"
#include "landmark_graph.h"

class LandmarkGraphExhaust : public LandmarkFactory {
public:
    LandmarkGraphExhaust(LandmarkGraph::Options &options, Exploration *exploration);
    virtual ~LandmarkGraphExhaust() {}
private:
    void generate_landmarks();
};

#endif
