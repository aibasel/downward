#ifndef LANDMARKS_LANDMARK_GRAPH_RPG_EXHAUST_H
#define LANDMARKS_LANDMARK_GRAPH_RPG_EXHAUST_H

#include "landmark_factory.h"
#include "landmark_graph.h"

class LandmarkGraphExhaust : public LandmarkFactory {
public:
    LandmarkGraphExhaust(LandmarkGraph::Options &options, Exploration *exploration);
    virtual ~LandmarkGraphExhaust() {}
    static LandmarkGraph *create(const std::vector<std::string> &config, int start,
                                  int &end, bool dry_run);
private:
    void generate_landmarks();
};

#endif
