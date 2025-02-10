#ifndef CARTESIAN_ABSTRACTIONS_UTILS_LANDMARKS_H
#define CARTESIAN_ABSTRACTIONS_UTILS_LANDMARKS_H

#include "../utils/hash.h"

#include <memory>
#include <unordered_map>
#include <vector>

class AbstractTask;
struct FactPair;

namespace landmarks {
class LandmarkGraph;
class LandmarkNode;
}

namespace cartesian_abstractions {
using VarToValues = std::unordered_map<int, std::vector<int>>;

extern std::shared_ptr<landmarks::LandmarkGraph> get_landmark_graph(
    const std::shared_ptr<AbstractTask> &task);
extern std::vector<FactPair> get_fact_landmarks(
    const landmarks::LandmarkGraph &graph);

extern utils::HashMap<FactPair, landmarks::LandmarkNode *> get_fact_to_landmark_map(
    const std::shared_ptr<landmarks::LandmarkGraph> &graph);

/*
  Do a breadth-first search through the landmark graph ignoring
  duplicates. Start at the given node and collect for each variable the
  facts that have to be made true before the given node can be true for
  the first time.
*/
extern VarToValues get_prev_landmarks(
    const landmarks::LandmarkNode *node);
}

#endif
