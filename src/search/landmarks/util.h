#ifndef LANDMARKS_UTIL_H
#define LANDMARKS_UTIL_H

#include <unordered_map>
#include <vector>

class OperatorProxy;
class TaskProxy;

namespace landmarks {
class LandmarkNode;

std::unordered_map<int, int> _intersect(
    const std::unordered_map<int, int> &a,
    const std::unordered_map<int, int> &b);

bool _possibly_reaches_lm(const OperatorProxy &op,
                          const std::vector<std::vector<int>> &lvl_var,
                          const LandmarkNode *lmp);

OperatorProxy get_operator_or_axiom(const TaskProxy &task_proxy, int op_or_axiom_id);
int get_operator_or_axiom_id(const OperatorProxy &op);
}

#endif
