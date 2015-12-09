#ifndef OPTIONS_TYPE_NAMER_H
#define OPTIONS_TYPE_NAMER_H

/*
  TODO: Try to minimize these #includes. We currently need
  option_parser_util.h for ParseTree at least.
*/
#include "option_parser_util.h"
#include "registries.h"

#include <memory>
#include <string>
#include <typeindex>


/*
  TypeNamer prints out names of types.

  There is no default implementation for Typenamer<T>::name(): the
  template needs to be specialized for each type we want to support.
  However, we have a generic version below for shared_ptr<...> types,
  which are the ones we use for plugins.
*/
template<typename T>
struct TypeNamer {
    static std::string name();
};

/*
  Note: for plug-in types, we use TypeNamer<shared_ptr<T>>::name().
  One might be tempted to strip away the shared_ptr<...> here and use
  TypeNamer<T>::name() instead, but this has the disadvantage that
  typeid(T) requires T to be a complete type, while
  typeid(shared_ptr<T>) also accepts incomplete types.
*/
template<typename T>
struct TypeNamer<std::shared_ptr<T>> {
    static std::string name() {
        using TPtr = std::shared_ptr<T>;
        const PluginTypeInfo &type_info =
            PluginTypeRegistry::instance()->get(std::type_index(typeid(TPtr)));
        return type_info.get_type_name();
    }
};

/*
  The following partial specialization for raw pointers is legacy code.
  This can go away once all plugins use shared_ptr.
*/
template<typename T>
struct TypeNamer<T *> {
    static std::string name() {
        return TypeNamer<std::shared_ptr<T>>::name();
    }
};

template <>
struct TypeNamer<int> {
    static std::string name() {
        return "int";
    }
};

template <>
struct TypeNamer<bool> {
    static std::string name() {
        return "bool";
    }
};

template <>
struct TypeNamer<double> {
    static std::string name() {
        return "double";
    }
};

template <>
struct TypeNamer<std::string> {
    static std::string name() {
        return "string";
    }
};

template <>
struct TypeNamer<ParseTree> {
    static std::string name() {
        return "ParseTree (this just means the input is parsed at a later point. The real type is probably a search engine.)";
    }
};

template<typename T>
struct TypeNamer<std::vector<T>> {
    static std::string name() {
        return "list of " + TypeNamer<T>::name();
    }
};

#endif
