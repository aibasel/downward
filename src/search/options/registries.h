#ifndef OPTIONS_REGISTRIES_H
#define OPTIONS_REGISTRIES_H

#include "../utils/system.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace options {
class OptionParser;

// A Registry<T> maps a string to a T-factory.
template<typename T>
class Registry {
public:
    using Factory = std::function<T(OptionParser &)>;

    void insert(const std::string &key, Factory factory) {
        if (registered.count(key)) {
            std::cerr << "duplicate key in registry: " << key << std::endl;
            utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
        }
        registered[key] = factory;
    }

    bool contains(const std::string &key) const {
        return registered.find(key) != registered.end();
    }

    Factory get(const std::string &key) const {
        return registered.at(key);
    }

    std::vector<std::string> get_sorted_keys() const {
        std::vector<std::string> keys;
        for (auto it : registered) {
            keys.push_back(it.first);
        }
        sort(keys.begin(), keys.end());
        return keys;
    }

    static Registry<T> *instance() {
        static Registry<T> instance_;
        return &instance_;
    }

private:
    // Define this below public methods since it needs "Factory" typedef.
    std::unordered_map<std::string, Factory> registered;

    Registry() = default;
};


/*
  The plugin type info class contains meta-information for a given
  type of plugins (e.g. "SearchEngine" or "MergeStrategyFactory").
*/
class PluginTypeInfo {
    std::type_index type;

    /*
      The type name should be "user-friendly". It is for example used
      as the name of the wiki page that documents this plugin type.
      It follows wiki conventions (e.g. "Heuristic", "SearchEngine",
      "ShrinkStrategy").
    */
    std::string type_name;

    /*
      General documentation for the plugin type. This is included at
      the top of the wiki page for this plugin type.
    */
    std::string documentation;
public:
    PluginTypeInfo(const std::type_index &type,
                   const std::string &type_name,
                   const std::string &documentation)
        : type(type),
          type_name(type_name),
          documentation(documentation) {
    }

    ~PluginTypeInfo() {
    }

    const std::type_index &get_type() const {
        return type;
    }

    const std::string &get_type_name() const {
        return type_name;
    }

    const std::string &get_documentation() const {
        return documentation;
    }
};

/*
  The plugin type registry collects information about all plugin types
  in use and gives access to the underlying information. This is used,
  for example, to generate the complete help output.

  Note that the information for individual plugins (rather than plugin
  types) is organized in separate registries, one for each plugin
  type. For example, there is a Registry<Heuristic> that organizes the
  Heuristic plugins.
*/

// TODO: Reduce code duplication with Registry<T>.
class PluginTypeRegistry {
    using Map = std::map<std::type_index, PluginTypeInfo>;
    PluginTypeRegistry() = default;
    ~PluginTypeRegistry() = default;
    Map registry;
public:
    static PluginTypeRegistry *instance();
    void insert(const PluginTypeInfo &info);
    const PluginTypeInfo &get(const std::type_index &type) const;

    Map::const_iterator begin() const {
        /*
          TODO (post-issue586): We want plugin types sorted by name in
          output. One way to achieve this is by defining the map's
          comparison function to sort first by the name and then by
          the type_index, but this is actually a bit difficult if the
          name isn't part of the key. One option to work around this
          is to use a set instead of a map as the internal data
          structure here.
        */
        return registry.cbegin();
    }

    Map::const_iterator end() const {
        return registry.cend();
    }
};
}

#endif
