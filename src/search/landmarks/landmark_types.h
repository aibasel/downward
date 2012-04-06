#ifndef LANDMARKS_LANDMARK_TYPES_H
#define LANDMARKS_LANDMARK_TYPES_H

#include "../utilities.h"

#include <utility>
#include <ext/hash_set>
#include <tr1/functional>

class hash_pointer {
public:
    size_t operator()(const void *p) const {
        //return size_t(reinterpret_cast<int>(p));
        std::tr1::hash<const void *> my_hash_class;
        return my_hash_class(p);
    }
};


typedef __gnu_cxx::hash_set<std::pair<int, int>, hash_int_pair> lm_set;
#endif
