#ifndef OPTIONS_PREDEFINITIONS_H
#define OPTIONS_PREDEFINITIONS_H

#include <string>
#include <unordered_map>

namespace options {
/*
  Predefinitions<T> maps strings to pointers to already created plug-in objects.
*/
template<typename T>
class Predefinitions {
    std::unordered_map<std::string, T> predefined;

    Predefinitions<T>() = default;

public:
    void predefine(std::string key, T object) {
        predefined[key] = object;
    }

    bool contains(const std::string &key) const {
        return predefined.find(key) != predefined.end();
    }

    T get(const std::string &key) const {
        return predefined.at(key);
    }

    static Predefinitions<T> *instance() {
        static Predefinitions<T> instance_;
        return &instance_;
    }
};
}

#endif
