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

extern std::unique_ptr<Landmarks::LandmarkGraph> get_landmark_graph();
extern std::vector<Fact> get_fact_landmarks(const Landmarks::LandmarkGraph &graph);
extern VarToValues get_prev_landmarks(const Landmarks::LandmarkGraph &graph, Fact fact);

extern void dump_landmark_graph(const Landmarks::LandmarkGraph &graph);
extern void write_landmark_graph_dot_file(const Landmarks::LandmarkGraph &graph);
}

#endif
