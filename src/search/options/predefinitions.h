#ifndef OPTIONS_PREDEFINITIONS_H
#define OPTIONS_PREDEFINITIONS_H

#include "any.h"
#include "errors.h"

#include "../utils/system.h"

#include <iostream>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace options {
class Predefinitions {
    std::unordered_map<std::string, std::pair<std::type_index, Any>> predefined;
public:
    Predefinitions() = default;

    template<typename T>
    void predefine(const std::string &key, T object) {
        if (predefined.count(key)) {
            std::cerr << "Multiple predefinitions for the key: "
                      << key << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        predefined.emplace(key, std::make_pair(std::type_index(typeid(T)), object));
    }

    template<typename T>
    bool contains(const std::string &key) const {
        std::type_index type(typeid(T));
        auto it = predefined.find(key);
        if (it == predefined.end()) {
            return false;
        } else if (it->second.first == type) {
            return true;
        } else {
            std::cerr << "Tried to look up a predefinition with a wrong type: "
                      << key << "(type: " << typeid(T).name()
                      << ")" << std::endl;
            exit_with_demangling_hint(utils::ExitCode::SEARCH_CRITICAL_ERROR, typeid(T).name());
        }
    }

    template<typename T>
    T get(const std::string &key) const {
        try {
            return any_cast<T>(predefined.at(key).second);
        } catch (BadAnyCast &) {
            std::cerr << "Tried to look up a predefinition with a wrong type: "
                      << key << "(type: " << typeid(T).name()
                      << ")" << std::endl;
            exit_with_demangling_hint(utils::ExitCode::SEARCH_CRITICAL_ERROR, typeid(T).name());
        }
    }
};
}

#endif
