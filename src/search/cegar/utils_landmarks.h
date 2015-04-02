#ifndef CEGAR_UTILS_LANDMARKS_H
#define CEGAR_UTILS_LANDMARKS_H

#include "utils.h"

#include <memory>
#include <vector>

namespace cegar {
std::shared_ptr<LandmarkGraph> get_landmark_graph();
std::vector<Fact> get_fact_landmarks(std::shared_ptr<LandmarkGraph> landmark_graph);
VariableToValues get_prev_landmarks(std::shared_ptr<LandmarkGraph>, Fact fact);

void write_landmark_graph(std::shared_ptr<LandmarkGraph> graph);
}

#endif
