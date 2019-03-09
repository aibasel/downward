#ifndef OPTIONS_PLUGIN_H
#define OPTIONS_PLUGIN_H

#include "doc_utils.h"
#include "option_parser.h"
#include "raw_registry.h"
#include "type_namer.h"

#include "../utils/strings.h"
#include "../utils/system.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>

namespace options {
class Registry;

/*
  The following function is not meant for users, but only for the
  plugin implementation. We only declare it here because the template
  implementations need it.
*/
extern void register_plugin_type_plugin(
    const std::type_info &type,
    const std::string &type_name,
    const std::string &documentation);


template<typename T>
class PluginTypePlugin {
public:
    PluginTypePlugin(
        const std::string &type_name,
        const std::string &documentation,
        const std::string &predefinition_key = "",
        const std::string &alias = "") {
        using TPtr = std::shared_ptr<T>;
        assert(!predefinition_key.empty() || alias.empty());
        assert(!utils::startswith(predefinition_key, "--"));
        assert(!utils::startswith(alias, "--"));

        PredefinitionFunction predefinition_function = predefinition_key.empty() ?
            nullptr : predefine_plugin<T>;

        RawRegistry::instance()->insert_plugin_type_data(
            std::type_index(typeid(TPtr)), type_name, documentation,
            predefinition_key, alias, predefinition_function);
    }

    ~PluginTypePlugin() = default;

    PluginTypePlugin(const PluginTypePlugin &other) = delete;
};


class PluginGroupPlugin {
public:
    PluginGroupPlugin(const std::string &group_id,
                      const std::string &doc_title);
    ~PluginGroupPlugin() = default;

    PluginGroupPlugin(const PluginGroupPlugin &other) = delete;
};

template<typename T>
class Plugin {
public:
    Plugin(
        const std::string &key,
        typename std::function<std::shared_ptr<T>(OptionParser &)> factory,
        const std::string &group = "") {
        using TPtr = std::shared_ptr<T>;
        PluginTypeNameGetter type_name_factory = [&](const Registry &registry) {
                return TypeNamer<TPtr>::name(registry);
            };
        DocFactory doc_factory = [factory](OptionParser &parser) {
                factory(parser);
            };
        std::type_index type(typeid(TPtr));
        RawRegistry::instance()->insert_plugin_data(
            key, factory, group, type_name_factory, doc_factory,
            type);
    }
    ~Plugin() = default;
    Plugin(const Plugin<T> &other) = delete;
};
}

#endif
