#ifndef OPTIONS_PLUGIN_H
#define OPTIONS_PLUGIN_H

#include "doc_utils.h"
#include "raw_registry.h"
#include "type_namer.h"

#include "../utils/strings.h"
#include "../utils/system.h"

#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>

namespace options {
class Predefinitions;
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
        const PredefinitionConfig &predefine = {
            {}, [](const std::string &, Registry &, Predefinitions &, bool) {}
        }) {
        using TPtr = std::shared_ptr<T>;
        for (const std::string &predefine_arg : predefine.first) {
            if (!utils::startswith(predefine_arg, "--")) {
                std::cerr << "Predefinition definition " << predefine_arg
                          << "does not start with '--'." << std::endl;
                utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
            }
        }
        RawRegistry::instance()->insert_plugin_type_data(
            std::type_index(typeid(TPtr)), type_name, documentation,
            predefine);
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
