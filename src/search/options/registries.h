#ifndef OPTIONS_REGISTRIES_H
#define OPTIONS_REGISTRIES_H

#include "../utils/system.h"

#include <algorithm>
#include <functional>
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
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
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
                   const std::string &documentation);

    ~PluginTypeInfo() = default;

    const std::type_index &get_type() const;
    const std::string &get_type_name() const;
    const std::string &get_documentation() const;

    bool operator<(const PluginTypeInfo &other) const;
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
    void insert(const PluginTypeInfo &info);
    const PluginTypeInfo &get(const std::type_index &type) const;
    std::vector<PluginTypeInfo> get_sorted_types() const;

    static PluginTypeRegistry *instance();
};


/*
  The plugin group registry collects information about plugin groups.
  A plugin group is a set of plugins (which should be of the same
  type, although the code does not enforce this) that should be
  grouped together in user documentation.

  For example, all PDB heuristics could be grouped together so that
  the documention page for the heuristics looks nicer.
*/

// TODO: Reduce code duplication with Registry<T>.
struct PluginGroupInfo {
    std::string group_id;
    std::string doc_title;
};

class PluginGroupRegistry {
    using Map = std::map<std::string, PluginGroupInfo>;
    PluginGroupRegistry() = default;
    ~PluginGroupRegistry() = default;
    Map registry;
public:
    void insert(const PluginGroupInfo &info);
    const PluginGroupInfo &get(const std::string &key) const;

    static PluginGroupRegistry *instance();
};
}

#endif
