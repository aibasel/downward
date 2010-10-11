#ifndef LANDMARKS_LANDMARKS_TYPES_H
#define LANDMARKS_LANDMARKS_TYPES_H

#include <utility>
#include <ext/hash_set>
#include <tr1/functional>

class hash_int_pair {
public:
    size_t operator()(const std::pair<int, int> &key) const {
        return size_t(1337 * key.first + key.second);
    }
};


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
