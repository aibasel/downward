#ifndef LANDMARKS_LANDMARKS_GRAPH_RPG_EXHAUST_H
#define LANDMARKS_LANDMARKS_GRAPH_RPG_EXHAUST_H

#include "landmarks_graph.h"

class LandmarksGraphExhaust {
public:
    LandmarksGraphExhaust(LandmarksGraph::LandmarkGraphOptions &options, Exploration *exploration);
    ~LandmarksGraphExhaust() {}
    // TODO: get_lm_graph *must* be called to avoid memory leeks!
    // returns a landmargraph created by HMLandmarks. take care to delete the pointer when you don't need it anymore!
    LandmarksGraph *get_lm_graph();
    static LandmarksGraph *create(const std::vector<std::string> &config, int start,
                                  int &end, bool dry_run);
private:
    LandmarksGraph *lm_graph;
    void generate_landmarks();
};

#endif
