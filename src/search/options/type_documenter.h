#ifndef OPTIONS_TYPE_DOCUMENTER_H
#define OPTIONS_TYPE_DOCUMENTER_H

#include "registries.h"

#include <memory>
#include <string>
#include <typeindex>

namespace options {
/*
  TypeDocumenter prints out the documentation synopsis for plug-in types.

  The same comments as for TypeNamer apply.
*/
template<typename T>
struct TypeDocumenter {
    static std::string synopsis() {
        /*
          TODO (post-issue586): once all plugin types are pluginized, this
          default implementation can go away (as in TypeNamer).
        */
        return "";
    }
};

// See comments for TypeNamer.
template<typename T>
struct TypeDocumenter<std::shared_ptr<T>> {
    static std::string synopsis() {
        using TPtr = std::shared_ptr<T>;
        const PluginTypeInfo &type_info =
            PluginTypeRegistry::instance()->get(std::type_index(typeid(TPtr)));
        return type_info.get_documentation();
    }
};

/*
  The following partial specialization for raw pointers is legacy code.
  This can go away once all plugins use shared_ptr.
*/
template<typename T>
struct TypeDocumenter<T *> {
    static std::string synopsis() {
        return TypeDocumenter<std::shared_ptr<T>>::synopsis();
    }
};
}

#endif
