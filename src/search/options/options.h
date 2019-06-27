#ifndef OPTIONS_OPTIONS_H
#define OPTIONS_OPTIONS_H

#include "any.h"
#include "errors.h"
#include "type_namer.h"

#include "../utils/system.h"

#include <string>
#include <typeinfo>
#include <unordered_map>

namespace options {
// Wrapper for unordered_map<string, Any>.
class Options {
    std::unordered_map<std::string, Any> storage;
    std::string unparsed_config;
    const bool help_mode;

public:
    explicit Options(bool help_mode = false);

    template<typename T>
    void set(const std::string &key, T value) {
        storage[key] = value;
    }

    template<typename T>
    T get(const std::string &key) const {
        const auto it = storage.find(key);
        if (it == storage.end()) {
            ABORT_WITH_DEMANGLING_HINT(
                "Attempt to retrieve nonexisting object of name " + key +
                " (type: " + typeid(T).name() + ")", typeid(T).name());
        }
        try {
            T result = any_cast<T>(it->second);
            return result;
        } catch (const BadAnyCast &) {
            ABORT_WITH_DEMANGLING_HINT(
                "Invalid conversion while retrieving config options!\n" +
                key + " is not of type " + typeid(T).name(), typeid(T).name());
        }
    }

    template<typename T>
    T get(const std::string &key, const T &default_value) const {
        if (storage.count(key))
            return get<T>(key);
        else
            return default_value;
    }

    template<typename T>
    void verify_list_non_empty(const std::string &key) const {
        if (!help_mode) {
            if (get_list<T>(key).empty()) {
                throw OptionParserError("Error: list for key " +
                                        key + " must not be empty\n");
            }
        }
    }

    template<typename T>
    std::vector<T> get_list(const std::string &key) const {
        return get<std::vector<T>>(key);
    }

    int get_enum(const std::string &key) const;
    bool contains(const std::string &key) const;
    const std::string &get_unparsed_config() const;
    void set_unparsed_config(const std::string &config);
};
}

#endif
