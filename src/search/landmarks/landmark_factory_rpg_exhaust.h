#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_EXHAUST_H

#include "landmark_factory.h"
#include "landmark_graph.h"

class LandmarkFactoryRpgExhaust : public LandmarkFactory {
public:
    LandmarkFactoryRpgExhaust(LandmarkGraph::Options &options, Exploration *exploration);
    virtual ~LandmarkFactoryRpgExhaust() {}
private:
    void generate_landmarks();
};

#endif
