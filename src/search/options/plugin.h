#ifndef OPTIONS_PLUGIN_H
#define OPTIONS_PLUGIN_H

#include "doc_store.h"
#include "registries.h"
#include "type_namer.h"

#include <memory>
#include <string>
#include <typeinfo>

namespace options {
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
    PluginTypePlugin(const std::string &type_name,
                     const std::string &documentation) {
        using TPtr = std::shared_ptr<T>;
        register_plugin_type_plugin(typeid(TPtr), type_name, documentation);
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
        typename Registry<std::shared_ptr<T>>::Factory factory,
        const std::string &group = "") {
        using TPtr = std::shared_ptr<T>;
        Registry<TPtr>::instance()->insert(key, factory);
        /*
          We cannot collect the plugin documentation here because this might
          require information from a TypePlugin object that has not yet been
          constructed. We therefore collect the necessary functions here and
          call them later, after all PluginType objects have been constructed.
        */
        DocFactory doc_factory = [factory](OptionParser &parser) {
                factory(parser);
            };
        PluginTypeNameGetter type_name_factory = [&]() {
                return TypeNamer<TPtr>::name();
            };
        DocStore::instance()->register_plugin(key, doc_factory, type_name_factory, group);
    }
    ~Plugin() = default;
    Plugin(const Plugin<T> &other) = delete;
};
}

#endif
