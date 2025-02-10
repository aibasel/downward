#ifndef PLUGINS_OPTIONS_H
#define PLUGINS_OPTIONS_H

#include "any.h"

#include "../utils/language.h"
#include "../utils/logging.h"
#include "../utils/system.h"

#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace plugins {
/*
  Elements in the storage of the class Options are stored as Any. Normally,
  the type stored inside the Any objects matches the type we eventually use
  to extract the element but there are two exceptions.

  Lists are stored as vector<Any> inside the options, so a normal any_cast
  will not work for them. We instead have to create a new vector and copy
  the casted elements into it. Since the new vector has to live somewhere,
  we do not support this for pointer to such vectors. Those will also not
  be created by the parser.

  Enums are stored as ints, so we have to extract them as such and cast the
  int into the enum types afterwards.
*/
template<typename ValueType, typename = void>
struct OptionsAnyCaster {
    static ValueType cast(const Any &operand) {
        return any_cast<ValueType>(operand);
    }
};

template<typename ValueType>
struct OptionsAnyCaster<
    ValueType, typename std::enable_if<std::is_enum<ValueType>::value>::type> {
    static ValueType cast(const Any &operand) {
        // Enums set within the code (options.set()) are already the right ValueType...
        if (operand.type() == typeid(ValueType)) {
            return any_cast<ValueType>(operand);
        }
        // ... otherwise (Enums set over the command line) they are ints.
        return static_cast<ValueType>(any_cast<int>(operand));
    }
};

template<typename T>
struct OptionsAnyCaster<std::vector<T>> {
    static std::vector<T> cast(const Any &operand) {
        if (operand.type() == typeid(std::vector<T>)) {
            return any_cast<std::vector<T>>(operand);
        }
        // any_cast returns a copy here, not a reference.
        const std::vector<Any> any_elements = any_cast<std::vector<Any>>(operand);
        std::vector<T> result;
        result.reserve(any_elements.size());
        for (const Any &element : any_elements) {
            result.push_back(OptionsAnyCaster<T>::cast(element));
        }
        return result;
    }
};

// Wrapper for unordered_map<string, Any>.
class Options {
    std::unordered_map<std::string, Any> storage;
    std::string unparsed_config;
public:
    explicit Options();
    /*
      TODO: we only need the copy constructor for cases where we need to modify
      the options after parsing (see merge_and_shrink_heuristic.cc for an
      example). This should no longer be necessary once we switch to builders.
      At this time, the constructor can probably be deleted.
    */
    Options(const Options &other) = default;

    template<typename T>
    void set(const std::string &key, T value) {
        storage[key] = value;
    }

    template<typename T>
    T get(const std::string &key) const {
        const auto it = storage.find(key);
        if (it == storage.end()) {
            ABORT(
                "Attempt to retrieve nonexisting object of name " + key +
                " (type: " + utils::get_type_name<T>() + ")");
        }
        try {
            T result = OptionsAnyCaster<T, void>::cast(it->second);
            return result;
        } catch (const BadAnyCast &) {
            ABORT(
                "Invalid conversion while retrieving config options!\n" +
                key + " is not of type " + utils::get_type_name<T>() +
                " but of type " + it->second.type_name());
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
    std::vector<T> get_list(const std::string &key) const {
        return get<std::vector<T>>(key);
    }

    bool contains(const std::string &key) const;
    const std::string &get_unparsed_config() const;
    void set_unparsed_config(const std::string &config);
};
}

#endif
