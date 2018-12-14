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
            std::cerr << key << " is already used in a predefinition." << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        predefined.emplace(key, std::make_pair(std::type_index(typeid(T)), object));
    }

    bool contains(const std::string &key) const {
        return predefined.find(key) != predefined.end();
    }

    template<typename T>
    T get(const std::string &key) const {
        try {
            return any_cast<T>(predefined.at(key).second);
        } catch (BadAnyCast &) {
            std::cerr << "Tried to look up a predefinition with a wrong type: "
                      << key << "(type: " << typeid(T).name()
                      << ")" << std::endl;
            // exit_with_demangling_hint(utils::ExitCode::SEARCH_CRITICAL_ERROR, typeid(T).name());
            throw OptionParserError("Parse Error.");
        }
    }

    template<typename T>
    T get(const std::string &key, const T &default_value) const {
        return (!contains(key)) ? default_value : get<T>(key);
    }
};
}

#endif
