#ifndef OPTIONS_DOC_UTILS_H
#define OPTIONS_DOC_UTILS_H

#include "bounds.h"

#include <functional>
#include <map>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

namespace options {
class OptionParser;
class Registry;

// See comment in constructor of Plugin in plugin.h.
using DocFactory = std::function<void (OptionParser &)>;
using PluginTypeNameGetter = std::function<std::string()>;

using ValueExplanations = std::vector<std::pair<std::string, std::string>>;


struct ArgumentInfo {
    std::string key;
    std::string help;
    std::string type_name;
    std::string default_value;
    Bounds bounds;
    ValueExplanations value_explanations;

    ArgumentInfo(
        const std::string &key,
        const std::string &help,
        const std::string &type_name,
        const std::string &default_value,
        const Bounds &bounds,
        const ValueExplanations &value_explanations)
        : key(key),
          help(help),
          type_name(type_name),
          default_value(default_value),
          bounds(bounds),
          value_explanations(value_explanations) {
    }
};


struct PropertyInfo {
    std::string property;
    std::string description;

    PropertyInfo(const std::string &property, const std::string &description)
        : property(property),
          description(description) {
    }
};


struct NoteInfo {
    std::string name;
    std::string description;
    bool long_text;

    NoteInfo(const std::string &name, const std::string &description, bool long_text)
        : name(name),
          description(description),
          long_text(long_text) {
    }
};


struct LanguageSupportInfo {
    std::string feature;
    std::string description;

    LanguageSupportInfo(const std::string &feature, const std::string &description)
        : feature(feature),
          description(description) {
    }
};


// Store documentation for a plugin.
struct PluginInfo {
    DocFactory doc_factory;
    PluginTypeNameGetter type_name_factory;
    std::string key;
    std::string name;
    std::string synopsis;
    std::string group;
    std::vector<ArgumentInfo> arg_help;
    std::vector<PropertyInfo> property_help;
    std::vector<LanguageSupportInfo> support_help;
    std::vector<NoteInfo> notes;
    bool hidden;

    void fill_docs(Registry &registry);

    std::string get_type_name() const;
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


struct PluginGroupInfo {
    std::string group_id;
    std::string doc_title;
};
}

#endif
