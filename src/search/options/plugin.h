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



template<typename T>
class Plugin {
public:
    Plugin(const std::string &key, typename Registry<T *>::Factory factory) {
        Registry<T *>::instance()->insert(key, factory);
        // See comment in PluginShared.
        DocFactory doc_factory = static_cast<DocFactory>(factory);
        TypeNameFactory type_name_factory = [&]() {
                                                return TypeNamer<T *>::name();
                                            };
        DocStore::instance()->register_plugin(key, doc_factory, type_name_factory);
    }
    ~Plugin() = default;
    Plugin(const Plugin<T> &other) = delete;
};


// TODO: This class will replace Plugin once we no longer need to support raw pointers.
template<typename T>
class PluginShared {
public:
    PluginShared(const std::string &key, typename Registry<std::shared_ptr<T>>::Factory factory) {
        using TPtr = std::shared_ptr<T>;
        Registry<TPtr>::instance()->insert(key, factory);
        /* We can't collect the plugin documentation here since TypePlugins may
           be loaded after Plugins and we need the names of PluginTypes when
           documenting Plugins. We therefore collect the necessary functions and
           execute them once all PluginTypes are loaded. */
        DocFactory doc_factory = static_cast<DocFactory>(factory);
        TypeNameFactory type_name_factory = [&]() {
                                                return TypeNamer<TPtr>::name();
                                            };
        DocStore::instance()->register_plugin(key, doc_factory, type_name_factory);
    }
    ~PluginShared() = default;
    PluginShared(const PluginShared<T> &other) = delete;
};
}

#endif
