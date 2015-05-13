#ifndef CEGAR_UTILS_LANDMARKS_H
#define CEGAR_UTILS_LANDMARKS_H

#include "utils.h"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

class LandmarkGraph;

namespace cegar {
using VarToValues = std::unordered_map<int, std::vector<int> >;

std::shared_ptr<LandmarkGraph> get_landmark_graph();
std::vector<Fact> get_fact_landmarks(std::shared_ptr<LandmarkGraph> landmark_graph);
VarToValues get_prev_landmarks(std::shared_ptr<LandmarkGraph>, Fact fact);

void dump_landmark_graph(std::shared_ptr<LandmarkGraph> graph);
void write_landmark_graph_dot_file(std::shared_ptr<LandmarkGraph> graph);
}

#endif
