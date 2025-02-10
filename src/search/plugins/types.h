#ifndef PLUGINS_TYPES_H
#define PLUGINS_TYPES_H

#include "any.h"
#include "registry_types.h"

#include "../utils/language.h"
#include "../utils/strings.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace utils {
class Context;
}

namespace plugins {
class CategoryPlugin;

class Type {
public:
    virtual ~Type() = default;

    virtual bool operator==(const Type &other) const = 0;
    bool operator!=(const Type &other) const;

    virtual bool is_basic_type() const;
    virtual const std::type_index &get_basic_type_index() const;

    virtual bool is_feature_type() const;
    virtual bool supports_variable_binding() const;
    virtual std::string get_synopsis() const;

    virtual bool is_list_type() const;
    virtual bool has_nested_type() const;
    virtual const Type &get_nested_type() const;

    virtual bool is_enum_type() const;
    virtual int get_enum_index(const std::string &, utils::Context &) const;
    virtual const EnumInfo &get_documented_enum_values() const;

    virtual bool is_symbol_type() const;

    virtual bool can_convert_into(const Type &other) const;

    virtual std::string name() const = 0;
    virtual size_t get_hash() const = 0;
};

class BasicType : public Type {
    std::type_index type;
    std::string class_name;
public:
    explicit BasicType(std::type_index type, const std::string &class_name);
    virtual bool operator==(const Type &other) const override;
    virtual bool is_basic_type() const override;
    virtual const std::type_index &get_basic_type_index() const override;
    virtual bool can_convert_into(const Type &other) const override;
    virtual std::string name() const override;
    virtual size_t get_hash() const override;
};

class FeatureType : public Type {
    std::type_index pointer_type;
    std::string type_name;
    std::string synopsis;
    bool can_be_bound_to_variable;

public:
    FeatureType(std::type_index pointer_type, const std::string &type_name,
                const std::string &synopsis, bool supports_variable_binding);
    virtual bool operator==(const Type &other) const override;
    virtual bool is_feature_type() const override;
    virtual bool supports_variable_binding() const override;
    virtual std::string get_synopsis() const override;
    virtual std::string name() const override;
    virtual size_t get_hash() const override;
};

class ListType : public Type {
    const Type &nested_type;

public:
    ListType(const Type &nested_type);
    virtual bool operator==(const Type &other) const override;
    virtual bool is_list_type() const override;
    virtual bool has_nested_type() const override;
    virtual const Type &get_nested_type() const override;
    virtual bool can_convert_into(const Type &other) const override;
    virtual std::string name() const override;
    virtual size_t get_hash() const override;
};

class EmptyListType : public Type {
public:
    virtual bool operator==(const Type &other) const override;
    virtual bool is_list_type() const override;
    virtual bool can_convert_into(const Type &other) const override;
    virtual std::string name() const override;
    virtual size_t get_hash() const override;
};

class EnumType : public Type {
    std::type_index type;
    std::vector<std::string> values;
    EnumInfo documented_values;

public:
    EnumType(std::type_index type, const EnumInfo &documented_values);
    virtual bool operator==(const Type &other) const override;
    virtual bool is_enum_type() const override;
    virtual int get_enum_index(const std::string &value, utils::Context &context) const override;
    virtual const EnumInfo &get_documented_enum_values() const override;
    virtual std::string name() const override;
    virtual size_t get_hash() const override;
};

class SymbolType : public Type {
public:
    virtual bool operator==(const Type &other) const override;
    virtual bool is_symbol_type() const override;
    bool can_convert_into(const Type &other) const override;
    virtual std::string name() const override;
    virtual size_t get_hash() const override;
};

class TypeRegistry {
    template<typename T>
    struct TypeOf {
        static const Type &value(TypeRegistry &registry);
    };

    template<typename T>
    struct TypeOf<std::vector<T>> {
        static const Type &value(TypeRegistry &registry);
    };

    struct SemanticHash {
        size_t operator()(const Type *t) const {
            if (!t) {
                return 0;
            }
            return t->get_hash();
        }
    };
    struct SemanticEqual {
        size_t operator()(const Type *t1, const Type *t2) const {
            if (!t1 || !t2) {
                return t1 == t2;
            }
            return *t1 == *t2;
        }
    };

    std::unordered_map<std::type_index, std::unique_ptr<Type>> registered_types;
    std::unordered_map<const Type *, std::unique_ptr<ListType>,
                       SemanticHash, SemanticEqual> registered_list_types;
    template<typename T>
    void insert_basic_type(const std::string &name);
    const Type &get_nonlist_type(std::type_index type) const;
public:
    static BasicType NO_TYPE;
    static SymbolType SYMBOL_TYPE;
    static EmptyListType EMPTY_LIST_TYPE;

    TypeRegistry();

    const FeatureType &create_feature_type(const CategoryPlugin &plugin);
    const EnumType &create_enum_type(const EnumPlugin &plugin);
    const ListType &create_list_type(const Type &element_type);

    template<typename T>
    const Type &get_type();

    static TypeRegistry *instance() {
        static TypeRegistry instance_;
        return &instance_;
    }
};

template<typename T>
const Type &TypeRegistry::TypeOf<T>::value(TypeRegistry &registry) {
    return registry.get_nonlist_type(typeid(T));
}

template<typename T>
const Type &TypeRegistry::TypeOf<std::vector<T>>::value(TypeRegistry &registry) {
    return registry.create_list_type(registry.get_type<T>());
}

template<typename T>
const Type &TypeRegistry::get_type() {
    return TypeOf<T>::value(*this);
}

extern Any convert(const Any &value, const Type &from_type, const Type &to_type,
                   utils::Context &context);
}

#endif
