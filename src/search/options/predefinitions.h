#ifndef OPTIONS_PREDEFINITIONS_H
#define OPTIONS_PREDEFINITIONS_H

#include "any.h"

#include <string>
#include <typeindex>
#include <unordered_map>

namespace options {
/*
  Predefinitions<T> maps strings to pointers to already created plug-in objects.
*/
class Predefinitions {
    std::unordered_map<std::type_index, std::unordered_map<std::string, Any>> predefined;
public:
    Predefinitions() = default;

    template<typename T>
    void predefine(const std::string &key, T object) {
        predefined[std::type_index(typeid(T))][key] = object;
    }

    template<typename T>
    bool contains(const std::string &key) const {
        std::type_index type(typeid(T));
        return predefined.count(type) && predefined.at(type).count(key);
    }

    template<typename T>
    T get(const std::string &key) const {
        std::type_index type(typeid(T));
        return any_cast<T>(predefined.at(type).at(key));
    }
};
}

#endif
