#ifndef OPTIONS_PREDEFINITIONS_H
#define OPTIONS_PREDEFINITIONS_H

#include <algorithm>
#include <map>
#include <string>

namespace options {
/*
  Predefinitions<T> maps strings to pointers to already created
  plug-in objects.
*/
template<typename T>
class Predefinitions {
public:
    static Predefinitions<T> *instance() {
        static Predefinitions<T> instance_;
        return &instance_;
    }

    void predefine(std::string k, T obj) {
        transform(k.begin(), k.end(), k.begin(), ::tolower);
        predefined[k] = obj;
    }

    bool contains(const std::string &k) {
        return predefined.find(k) != predefined.end();
    }

    T get(const std::string &k) {
        return predefined[k];
    }

private:
    Predefinitions<T>() = default;
    std::map<std::string, T> predefined;
};
}

#endif
