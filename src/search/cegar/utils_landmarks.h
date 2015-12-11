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

// TODO: Add typedef for LandmarkGraph.

extern std::shared_ptr<Landmarks::LandmarkGraph> get_landmark_graph();
extern std::vector<Fact> get_fact_landmarks(
    std::shared_ptr<Landmarks::LandmarkGraph> landmark_graph);
extern VarToValues get_prev_landmarks(
    std::shared_ptr<Landmarks::LandmarkGraph>, Fact fact);

extern void dump_landmark_graph(
    std::shared_ptr<Landmarks::LandmarkGraph> graph);
extern void write_landmark_graph_dot_file(
    std::shared_ptr<Landmarks::LandmarkGraph> graph);
}

#endif
