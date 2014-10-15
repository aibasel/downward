#ifndef LANDMARKS_UTIL_H
#define LANDMARKS_UTIL_H

#include <vector>
#include <ext/hash_map>

struct GlobalCondition;
class GlobalOperator;
class LandmarkNode;

bool _possibly_fires(const std::vector<GlobalCondition> &conditions,
                     const std::vector<std::vector<int> > &lvl_var);

__gnu_cxx::hash_map<int, int> _intersect(
    const __gnu_cxx::hash_map<int, int> &a,
    const __gnu_cxx::hash_map<int, int> &b);

bool _possibly_reaches_lm(const GlobalOperator &o,
                          const std::vector<std::vector<int> > &lvl_var,
                          const LandmarkNode *lmp);

#endif
