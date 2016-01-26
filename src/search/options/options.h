#ifndef OPTIONS_OPTIONS_H
#define OPTIONS_OPTIONS_H

#include "any.h"
#include "type_namer.h"

#include "../utils/system.h"

#include <map>
#include <string>

namespace options {
//Options is just a wrapper for map<string, Any>
class Options {
public:
    Options(bool hm = false)
        : unparsed_config("<missing>"),
          help_mode(hm) {
    }

    void set_help_mode(bool hm) {
        help_mode = hm;
    }

    std::map<std::string, Any> storage;

    template<typename T>
    void set(std::string key, T value) {
        storage[key] = value;
    }

    template<typename T>
    T get(std::string key) const {
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
            std::cout << "Invalid conversion while retrieving config options!"
                      << std::endl
                      << key << " is not of type " << TypeNamer<T>::name()
                      << std::endl << "exiting" << std::endl;
            utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
        }
    }

    template<typename T>
    T get(std::string key, const T &default_value) const {
        if (storage.count(key))
            return get<T>(key);
        else
            return default_value;
    }

    template<typename T>
    void verify_list_non_empty(std::string key) const {
        if (!help_mode) {
            std::vector<T> temp_vec = get<std::vector<T>>(key);
            if (temp_vec.empty()) {
                std::cout << "Error: unexpected empty list!"
                          << std::endl
                          << "List " << key << " is empty"
                          << std::endl;
                utils::exit_with(utils::ExitCode::INPUT_ERROR);
            }
        }
    }

    template<typename T>
    std::vector<T> get_list(std::string key) const {
        return get<std::vector<T>>(key);
    }

    int get_enum(std::string key) const {
        return get<int>(key);
    }

    bool contains(std::string key) const {
        return storage.find(key) != storage.end();
    }

    std::string get_unparsed_config() const {
        return unparsed_config;
    }

    void set_unparsed_config(const std::string &config) {
        unparsed_config = config;
    }
private:
    std::string unparsed_config;
    bool help_mode;
};
}

#endif
