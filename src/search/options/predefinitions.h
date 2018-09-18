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

    Predefinitions() = default;

public:
    template<typename T>
    void predefine(std::string key, T object) {
        predefined[std::type_index(typeid(T))][key] = object;
    }

    template<typename T>
    bool contains(const std::string &key) const {
        std::type_index tmp(typeid(T));
        return predefined.find(tmp) != predefined.end()
               && predefined.at(tmp).find(key) != predefined.at(tmp).end();
    }

    template<typename T>
    T get(const std::string &key) const {
        std::type_index tmp(typeid(T));
        return any_cast<T>(predefined.at(tmp).at(key));
    }

    static Predefinitions *instance() {
        static Predefinitions instance_;
        return &instance_;
    }
};
}

#endif
