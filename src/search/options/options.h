#ifndef OPTIONS_OPTIONS_H
#define OPTIONS_OPTIONS_H

#include "any.h"
#include "type_namer.h"

#include "../utils/system.h"

#include <iostream>
#include <string>
#include <unordered_map>

namespace options {
// Wrapper for unordered_map<string, Any>.
class Options {
    std::unordered_map<std::string, Any> storage;
    std::string unparsed_config;
    const bool help_mode;

public:
    explicit Options(bool help_mode = false)
        : unparsed_config("<missing>"),
          help_mode(help_mode) {
    }

    template<typename T>
    void set(const std::string &key, T value) {
        storage[key] = value;
    }

    template<typename T>
    T get(const std::string &key) const {
        const auto it = storage.find(key);
        if (it == storage.end()) {
            ABORT("Attempt to retrieve nonexisting object of name " +
                  key + " (type: " + TypeNamer<T>::name() +
                  ") from options.");
        }
        try {
            T result = any_cast<T>(it->second);
            return result;
        } catch (const BadAnyCast &) {
            std::cerr << "Invalid conversion while retrieving config options!"
                      << std::endl
                      << key << " is not of type " << TypeNamer<T>::name()
                      << std::endl << "exiting" << std::endl;
            utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
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
                std::cerr << "Error: unexpected empty list!"
                          << std::endl
                          << "List " << key << " is empty"
                          << std::endl;
                utils::exit_with(utils::ExitCode::INPUT_ERROR);
            }
        }
    }

    template<typename T>
    std::vector<T> get_list(const std::string &key) const {
        return get<std::vector<T>>(key);
    }

    int get_enum(const std::string &key) const {
        return get<int>(key);
    }

    bool contains(const std::string &key) const {
        return storage.find(key) != storage.end();
    }

    const std::string &get_unparsed_config() const {
        return unparsed_config;
    }

    void set_unparsed_config(const std::string &config) {
        unparsed_config = config;
    }
};
}

#endif
