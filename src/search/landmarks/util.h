#ifndef LANDMARKS_UTIL_H
#define LANDMARKS_UTIL_H

#include <ext/hash_set>
#include <cassert>
#include <climits>
#include <ext/hash_map>
#include <ext/hash_set>
#include "../operator.h"
#include "landmarks_graph.h"

using namespace std;
using namespace __gnu_cxx;

bool _possibly_fires(const vector<Prevail> &prevail, const vector<vector<int> > &lvl_var);

hash_map<int, int> _intersect(const hash_map<int, int> &a, const hash_map<int, int> &b);

bool _possibly_reaches_lm(const Operator &o,
                          const vector<vector<int> > &lvl_var, const LandmarkNode *lmp);

#endif
