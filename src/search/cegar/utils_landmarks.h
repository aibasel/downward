#ifndef CEGAR_UTILS_LANDMARKS_H
#define CEGAR_UTILS_LANDMARKS_H

#include "utils.h"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Landmarks {
class LandmarkGraph;
}

namespace CEGAR {
using VarToValues = std::unordered_map<int, std::vector<int>>;
using SharedGraph = std::shared_ptr<Landmarks::LandmarkGraph>;

extern SharedGraph get_landmark_graph();
extern std::vector<Fact> get_fact_landmarks(SharedGraph landmark_graph);
extern VarToValues get_prev_landmarks(SharedGraph, Fact fact);

extern void dump_landmark_graph(SharedGraph graph);
extern void write_landmark_graph_dot_file(SharedGraph graph);
}

#endif
