#ifndef CEGAR_UTILS_LANDMARKS_H
#define CEGAR_UTILS_LANDMARKS_H

#include <memory>
#include <unordered_map>
#include <vector>

class AbstractTask;
struct FactPair;

namespace landmarks {
class LandmarkGraph;
}

namespace cegar {
using VarToValues = std::unordered_map<int, std::vector<int>>;

extern std::shared_ptr<landmarks::LandmarkGraph> get_landmark_graph(
    const std::shared_ptr<AbstractTask> &task);
extern std::vector<FactPair> get_fact_landmarks(
    const landmarks::LandmarkGraph &graph);

/*
  Do a breadth-first search through the landmark graph ignoring
  duplicates. Start at the node for the given fact and collect for each
  variable the facts that have to be made true before the given fact
  can be true for the first time.
*/
extern VarToValues get_prev_landmarks(
    const landmarks::LandmarkGraph &graph, const FactPair &fact);
}

#endif
