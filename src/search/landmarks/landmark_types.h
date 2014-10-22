#ifndef LANDMARKS_LANDMARK_TYPES_H
#define LANDMARKS_LANDMARK_TYPES_H

#include "../utilities.h"

#include <utility>
#include <ext/hash_set>

typedef __gnu_cxx::hash_set<std::pair<int, int>, hash_int_pair> lm_set;
#endif
