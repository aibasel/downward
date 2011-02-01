#ifndef LANDMARKS_LANDMARK_GRAPH_RPG_EXHAUST_H
#define LANDMARKS_LANDMARK_GRAPH_RPG_EXHAUST_H

#include "landmark_graph.h"

class LandmarkGraphExhaust {
public:
    LandmarkGraphExhaust(LandmarkGraph::Options &options, Exploration *exploration);
    ~LandmarkGraphExhaust() {}
    // TODO: get_lm_graph *must* be called to avoid memory leeks!
    // returns a landmargraph created by HMLandmark. take care to delete the pointer when you don't need it anymore!
    LandmarkGraph *get_lm_graph();
    static LandmarkGraph *create(const std::vector<std::string> &config, int start,
                                  int &end, bool dry_run);
private:
    LandmarkGraph *lm_graph;
    void generate_landmarks();
};

#endif
