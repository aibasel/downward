#ifndef OPTIONS_TYPE_NAMER_H
#define OPTIONS_TYPE_NAMER_H

#include "parse_tree.h"
#include "registries.h"

#include <memory>
#include <string>
#include <typeindex>

namespace options {
/*
  TypeNamer prints out names of types.

  There is no default implementation for TypeNamer<T>::name: the template needs
  to be specialized for each type we want to support. However, we have a
  generic version below for shared_ptr<...> types, which are the ones we use
  for plugins.
*/
template<typename T>
struct TypeNamer {
    static std::string name(const Registry &registry);
};

/*
  Note: for plug-in types, we use TypeNamer<shared_ptr<T>>::name. One might be
  tempted to strip away the shared_ptr<...> here and use TypeNamer<T>::name
  instead, but this has the disadvantage that typeid(T) requires T to be a
  complete type, while typeid(shared_ptr<T>) also accepts incomplete types.
*/
template<typename T>
struct TypeNamer<std::shared_ptr<T>> {
    static std::string name(const Registry &registry) {
        using TPtr = std::shared_ptr<T>;
        const PluginTypeInfo &type_info =
            registry.get_type_info(std::type_index(typeid(TPtr)));
        return type_info.get_type_name();
    }
};

template<>
struct TypeNamer<int> {
    static std::string name(const Registry &) {
        return "int";
    }
};

template<>
struct TypeNamer<bool> {
    static std::string name(const Registry &) {
        return "bool";
    }
};

template<>
struct TypeNamer<double> {
    static std::string name(const Registry &) {
        return "double";
    }
};

template<>
struct TypeNamer<std::string> {
    static std::string name(const Registry &) {
        return "string";
    }
};

template<>
struct TypeNamer<ParseTree> {
    static std::string name(const Registry &) {
        return "ParseTree (this just means the input is parsed at a later point."
               " The real type is probably a search engine.)";
    }
};

template<typename T>
struct TypeNamer<std::vector<T>> {
    static std::string name(const Registry &registry) {
        return "list of " + TypeNamer<T>::name(registry);
    }
};
}

#endif
