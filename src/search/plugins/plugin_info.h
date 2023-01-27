#ifndef PLUGINS_PLUGIN_INFO_H
#define PLUGINS_PLUGIN_INFO_H

#include "bounds.h"
#include "types.h"

#include <string>
#include <typeindex>
#include <vector>

namespace plugins {
struct ArgumentInfo {
    std::string key;
    std::string help;
    const Type &type;
    std::string default_value;
    Bounds bounds;
    static const std::string NO_DEFAULT;

    // TODO: once we switch to builder, this should no longer be necessary.
    bool lazy_construction;

    ArgumentInfo(
        const std::string &key,
        const std::string &help,
        const Type &type,
        const std::string &default_value,
        const Bounds &bounds,
        bool lazy_construction = false);

    bool is_optional() const;
    bool has_default() const;
};


struct PropertyInfo {
    std::string property;
    std::string description;

    PropertyInfo(const std::string &property, const std::string &description);
};


struct NoteInfo {
    std::string name;
    std::string description;
    bool long_text;

    NoteInfo(const std::string &name, const std::string &description, bool long_text);
};


struct LanguageSupportInfo {
    std::string feature;
    std::string description;

    LanguageSupportInfo(const std::string &feature, const std::string &description);
};
}

#endif
