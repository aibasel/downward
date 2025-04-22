#ifndef LANDMARKS_UTIL_H
#define LANDMARKS_UTIL_H

#include "../utils/hash.h"

#include <vector>

struct FactPair;
class OperatorProxy;
class TaskProxy;

namespace utils {
class LogProxy;
}

namespace landmarks {
class Landmark;
class LandmarkNode;
class LandmarkGraph;

extern bool possibly_reaches_landmark(
    const OperatorProxy &op, const std::vector<std::vector<bool>> &reached,
    const Landmark &landmark);

extern utils::HashSet<FactPair> get_intersection(
    const utils::HashSet<FactPair> &set1, const utils::HashSet<FactPair> &set2);
extern void union_inplace(utils::HashSet<FactPair> &set1,
                          const utils::HashSet<FactPair> &set2);

extern OperatorProxy get_operator_or_axiom(
    const TaskProxy &task_proxy, int op_or_axiom_id);
extern int get_operator_or_axiom_id(const OperatorProxy &op);

extern void dump_landmark_graph(
    const TaskProxy &task_proxy,
    const LandmarkGraph &graph,
    utils::LogProxy &log);
}

#endif
