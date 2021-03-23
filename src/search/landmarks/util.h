#ifndef LANDMARKS_UTIL_H
#define LANDMARKS_UTIL_H

#include <unordered_map>
#include <vector>

class OperatorProxy;
class TaskProxy;

namespace landmarks {
class LandmarkNode;
class LandmarkGraph;

extern std::unordered_map<int, int> _intersect(
    const std::unordered_map<int, int> &a,
    const std::unordered_map<int, int> &b);

extern bool _possibly_reaches_lm(
    const OperatorProxy &op, const std::vector<std::vector<int>> &lvl_var,
    const LandmarkNode *lmp);

extern OperatorProxy get_operator_or_axiom(const TaskProxy &task_proxy, int op_or_axiom_id);
extern int get_operator_or_axiom_id(const OperatorProxy &op);

extern void dump_landmark_graph(const TaskProxy &task_proxy, const LandmarkGraph &graph);
}

#endif
