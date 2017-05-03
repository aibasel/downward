#ifndef OPTIONS_DOC_STORE_H
#define OPTIONS_DOC_STORE_H

#include "bounds.h"

#include <map>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

namespace options {
using ValueExplanations = std::vector<std::pair<std::string, std::string>>;


class TypeInfo {
    std::type_index type;
    std::string type_name;

public:
    TypeInfo(const std::type_index &type, const std::string &type_name = "")
        : type(type),
          type_name(type_name) {
    }

    std::string get_type_name() const;
};


struct ArgumentInfo {
    std::string key;
    std::string help;
    TypeInfo type;
    std::string default_value;
    Bounds bounds;
    ValueExplanations value_explanations;

    ArgumentInfo(
        const std::string &key,
        const std::string &help,
        const TypeInfo &type,
        const std::string &default_value,
        const Bounds &bounds,
        const ValueExplanations &value_explanations)
        : key(key),
          help(help),
          type(type),
          default_value(default_value),
          bounds(bounds),
          value_explanations(value_explanations) {
    }

    std::string get_type_name() const;
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

    LanguageSupportInfo(const std::string &feat, const std::string &descr)
        : feature(feat),
          description(descr) {
    }
};


// Store documentation for a single type, for use in combination with DocStore.
struct DocStruct {
    TypeInfo type;
    std::string full_name;
    std::string synopsis;
    std::vector<ArgumentInfo> arg_help;
    std::vector<PropertyInfo> property_help;
    std::vector<LanguageSupportInfo> support_help;
    std::vector<NoteInfo> notes;
    bool hidden;

    // TODO: set all args in ctor.
    explicit DocStruct(const TypeInfo &type)
        : type(type) {
    }
};


// Store documentation for types parsed in help mode.
class DocStore {
    std::map<std::string, DocStruct> registered;

    DocStore() = default;

public:
    static DocStore *instance() {
        static DocStore instance_;
        return &instance_;
    }

    //void register_plugin_type(const std::string &key, const TypeInfo &type);

    void register_plugin(const std::string &key, const TypeInfo &type);

    void add_arg(
        const std::string &k,
        const std::string &arg_name,
        const std::string &help,
        const TypeInfo &type,
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

    DocStruct get(const std::string &key);

    std::vector<std::string> get_keys();

    std::vector<std::string> get_types();
};
}

#endif
