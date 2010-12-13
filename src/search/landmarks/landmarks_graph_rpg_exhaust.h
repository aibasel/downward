#ifndef LANDMARKS_LANDMARKS_GRAPH_RPG_EXHAUST_H
#define LANDMARKS_LANDMARKS_GRAPH_RPG_EXHAUST_H

#include "landmarks_graph.h"

class LandmarksGraphExhaust : public LandmarksGraph {
    void generate_landmarks();
public:
    LandmarksGraphExhaust(LandmarkGraphOptions &options, Exploration *exploration)
        : LandmarksGraph(options, exploration) {}
    ~LandmarksGraphExhaust() {}
    static LandmarksGraph *create(const std::vector<std::string> &config, int start,
                                  int &end, bool dry_run);
};

#endif
