#ifndef OPTIONS_DOC_STORE_H
#define OPTIONS_DOC_STORE_H

#include "bounds.h"

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace options {
class OptionParser;

// See comment in constructor of PluginShared in plugin.h.
using DocFactory = std::function<void(OptionParser &)>;
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
    std::string name;
    std::string synopsis;
    std::vector<ArgumentInfo> arg_help;
    std::vector<PropertyInfo> property_help;
    std::vector<LanguageSupportInfo> support_help;
    std::vector<NoteInfo> notes;
    bool hidden;

    void fill_docs();

    std::string get_type_name() const;
};


// Store documentation for types parsed in help mode.
class DocStore {
    std::map<std::string, PluginInfo> plugin_infos;

    DocStore() = default;

public:
    static DocStore *instance() {
        static DocStore instance_;
        return &instance_;
    }

    void register_plugin(
        const std::string &key, DocFactory factory, PluginTypeNameGetter type_name_factory);

    void add_arg(
        const std::string &key,
        const std::string &arg_name,
        const std::string &help,
        const std::string &type_name,
        const std::string &default_value,
        const Bounds &bounds,
        const ValueExplanations &value_explanations = ValueExplanations());

    void add_value_explanations(
        const std::string &key,
        const std::string &arg_name,
        const ValueExplanations &value_explanations);

    void set_synopsis(
        const std::string &key, const std::string &name, const std::string &description);

    void add_property(
        const std::string &key, const std::string &name, const std::string &description);

    void add_feature(
        const std::string &key, const std::string &feature, const std::string &description);

    void add_note(
        const std::string &key,
        const std::string &name,
        const std::string &description,
        bool long_text);

    void hide(const std::string &key);

    bool contains(const std::string &key);

    PluginInfo &get(const std::string &key);

    std::vector<std::string> get_keys();
};
}

#endif
