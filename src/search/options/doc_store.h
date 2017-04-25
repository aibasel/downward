#ifndef OPTIONS_DOC_STORE_H
#define OPTIONS_DOC_STORE_H

#include "bounds.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace options {
using ValueExplanations = std::vector<std::pair<std::string, std::string>>;


struct ArgumentInfo {
    std::string key;
    std::string help;
    std::string type_name;
    std::string default_value;
    Bounds bounds;
    std::vector<std::pair<std::string, std::string>> value_explanations;

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

    LanguageSupportInfo(const std::string &feat, const std::string &descr)
        : feature(feat),
          description(descr) {
    }
};


// Store documentation for a single type, for use in combination with DocStore.
struct DocStruct {
    std::string type;
    std::string full_name;
    std::string synopsis;
    std::vector<ArgumentInfo> arg_help;
    std::vector<PropertyInfo> property_help;
    std::vector<LanguageSupportInfo> support_help;
    std::vector<NoteInfo> notes;
    bool hidden;
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

    void register_object(std::string key, const std::string &type);

    void add_arg(
        const std::string &k,
        const std::string &arg_name,
        const std::string &help,
        const std::string &type,
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
