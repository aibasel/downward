#ifndef LANDMARKS_LANDMARKS_GRAPH_RPG_EXHAUST_H
#define LANDMARKS_LANDMARKS_GRAPH_RPG_EXHAUST_H

#include "landmarks_graph.h"

class LandmarksGraphExhaust : public LandmarksGraph {
    void generate_landmarks();
public:
    LandmarksGraphExhaust(Exploration* exploration) : LandmarksGraph(exploration) {}
    ~LandmarksGraphExhaust(){}
};

#endif
