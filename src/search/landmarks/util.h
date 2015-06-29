#ifndef LANDMARKS_UTIL_H
#define LANDMARKS_UTIL_H

#include <unordered_map>
#include <vector>

struct GlobalCondition;
class GlobalOperator;
class LandmarkNode;

bool _possibly_fires(const std::vector<GlobalCondition> &conditions,
                     const std::vector<std::vector<int> > &lvl_var);

std::unordered_map<int, int> _intersect(
    const std::unordered_map<int, int> &a,
    const std::unordered_map<int, int> &b);

bool _possibly_reaches_lm(const GlobalOperator &o,
                          const std::vector<std::vector<int> > &lvl_var,
                          const LandmarkNode *lmp);

#endif
