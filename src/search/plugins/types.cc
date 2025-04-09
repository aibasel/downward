#include "types.h"

#include "plugin.h"

#include "../utils/strings.h"

#include <sstream>
#include <typeindex>

using namespace std;

namespace plugins {
bool Type::operator!=(const Type &other) const {
    return !(*this == other);
}

bool Type::is_basic_type() const {
    return false;
}

const type_index &Type::get_basic_type_index() const {
    ABORT("Used Type::get_basic_type_index on a type that does not support it.");
}

bool Type::is_feature_type() const {
    return false;
}

bool Type::supports_variable_binding() const {
    return false;
}

string Type::get_synopsis() const {
    ABORT("Used Type::get_synopsis on a type that does not support it.");
}

bool Type::is_list_type() const {
    return false;
}

bool Type::has_nested_type() const {
    return false;
}

const Type &Type::get_nested_type() const {
    ABORT("Used Type::get_nested_type on a type that does not support it.");
}

bool Type::is_enum_type() const {
    return false;
}

int Type::get_enum_index(const string &, utils::Context &) const {
    ABORT("Used Type::get_enum_index on a type that does not support it.");
}

const EnumInfo &Type::get_documented_enum_values() const {
    ABORT("Used Type::get_documented_enum_values on a type that does not support it.");
}

bool Type::is_symbol_type() const {
    return false;
}

bool Type::can_convert_into(const Type &other) const {
    return *this == other;
}

BasicType::BasicType(type_index type, const string &class_name)
    : type(type), class_name(class_name) {
}

bool BasicType::operator==(const Type &other) const {
    const BasicType *other_ptr = dynamic_cast<const BasicType *>(&other);
    return other_ptr && type == other_ptr->type;
}

bool BasicType::is_basic_type() const {
    return true;
}

const type_index &BasicType::get_basic_type_index() const {
    return type;
}

bool BasicType::can_convert_into(const Type &other) const {
    return Type::can_convert_into(other)
           || (other.is_basic_type()
               && get_basic_type_index() == typeid(int)
               && other.get_basic_type_index() == typeid(double));
}

string BasicType::name() const {
    return class_name;
}

size_t BasicType::get_hash() const {
    return hash<type_index>()(type);
}

FeatureType::FeatureType(type_index pointer_type, const string &type_name,
                         const string &synopsis, bool supports_variable_binding)
    : pointer_type(pointer_type), type_name(type_name), synopsis(synopsis),
      can_be_bound_to_variable(supports_variable_binding) {
}

bool FeatureType::operator==(const Type &other) const {
    const FeatureType *other_ptr = dynamic_cast<const FeatureType *>(&other);
    return other_ptr && pointer_type == other_ptr->pointer_type;
}

bool FeatureType::is_feature_type() const {
    return true;
}

bool FeatureType::supports_variable_binding() const {
    return can_be_bound_to_variable;
}

string FeatureType::get_synopsis() const {
    return synopsis;
}

string FeatureType::name() const {
    return type_name;
}

size_t FeatureType::get_hash() const {
    return hash<type_index>()(typeid(FeatureType)) ^ hash<type_index>()(pointer_type);
}

ListType::ListType(const Type &nested_type)
    : nested_type(nested_type) {
}

bool ListType::operator==(const Type &other) const {
    const ListType *other_ptr = dynamic_cast<const ListType *>(&other);
    return other_ptr && nested_type == other_ptr->nested_type;
}

bool ListType::is_list_type() const {
    return true;
}

bool ListType::has_nested_type() const {
    return true;
}

const Type &ListType::get_nested_type() const {
    return nested_type;
}

bool ListType::can_convert_into(const Type &other) const {
    return other.is_list_type() && other.has_nested_type()
           && nested_type.can_convert_into(other.get_nested_type());
}

string ListType::name() const {
    return "list of " + nested_type.name();
}

size_t ListType::get_hash() const {
    return hash<type_index>()(typeid(ListType)) ^ nested_type.get_hash();
}

bool EmptyListType::operator==(const Type &other) const {
    const EmptyListType *other_ptr = dynamic_cast<const EmptyListType *>(&other);
    return other_ptr;
}

bool EmptyListType::is_list_type() const {
    return true;
}

bool EmptyListType::can_convert_into(const Type &other) const {
    return other.is_list_type();
}

string EmptyListType::name() const {
    return "empty list";
}

size_t EmptyListType::get_hash() const {
    return hash<type_index>()(typeid(EmptyListType));
}

EnumType::EnumType(type_index type, const EnumInfo &documented_values)
    : type(type), documented_values(documented_values) {
    values.reserve(documented_values.size());
    for (const auto &value_and_doc : documented_values) {
        values.push_back(utils::tolower(value_and_doc.first));
    }
}

bool EnumType::operator==(const Type &other) const {
    const EnumType *other_ptr = dynamic_cast<const EnumType *>(&other);
    return other_ptr && type == other_ptr->type;
}

bool EnumType::is_enum_type() const {
    return true;
}

int EnumType::get_enum_index(const string &value, utils::Context &context) const {
    auto it = find(values.begin(), values.end(), value);
    int enum_index = static_cast<int>(it - values.begin());
    if (enum_index >= static_cast<int>(values.size())) {
        ostringstream message;
        message << "Invalid enum value: " << value << endl
                << "Options: " << utils::join(values, ", ");
        context.error(message.str());
    }
    return enum_index;
}

const EnumInfo &EnumType::get_documented_enum_values() const {
    return documented_values;
}

string EnumType::name() const {
    return "{" + utils::join(values, ", ") + "}";
}

size_t EnumType::get_hash() const {
    size_t hash_value = 0;
    for (const string &value : values) {
        hash_value ^= hash<string>()(value);
    }
    return hash_value;
}


bool SymbolType::operator==(const Type &other) const {
    return other.is_symbol_type();
}

bool SymbolType::is_symbol_type() const {
    return true;
}

bool SymbolType::can_convert_into(const Type &other) const {
    return (*this == other) || other.is_enum_type();
}

string SymbolType::name() const {
    return "symbol";
}

size_t SymbolType::get_hash() const {
    return hash<type_index>()(typeid(SymbolType));
}

Any convert(const Any &value, const Type &from_type, const Type &to_type, utils::Context &context) {
    if (from_type == to_type) {
        return value;
    } else if (from_type.is_basic_type() && from_type.get_basic_type_index() == typeid(int)
               && to_type.is_basic_type() && to_type.get_basic_type_index() == typeid(double)) {
        int int_value = any_cast<int>(value);
        if (int_value == numeric_limits<int>::max()) {
            return Any(numeric_limits<double>::infinity());
        } else if (int_value == numeric_limits<int>::min()) {
            return Any(-numeric_limits<double>::infinity());
        }
        return Any(static_cast<double>(int_value));
    } else if (from_type.is_symbol_type() && to_type.is_enum_type()) {
        string str_value = any_cast<string>(value);
        return Any(to_type.get_enum_index(str_value, context));
    } else if (from_type.is_list_type() && !from_type.has_nested_type() && to_type.is_list_type()) {
        /* A list without a specified type for its nested elements can be
           interpreted as a list of any other type. */
        return value;
    } else if (from_type.is_list_type() && from_type.has_nested_type()
               && to_type.is_list_type() && to_type.has_nested_type()
               && from_type.get_nested_type().can_convert_into(to_type.get_nested_type())) {
        const Type &from_nested_type = from_type.get_nested_type();
        const Type &to_nested_type = to_type.get_nested_type();
        const vector<Any> &elements = any_cast<vector<Any>>(value);
        vector<Any> converted_elements;
        converted_elements.reserve(elements.size());
        for (const Any &element : elements) {
            converted_elements.push_back(convert(element, from_nested_type, to_nested_type, context));
        }
        return Any(converted_elements);
    } else {
        ABORT("Cannot convert " + from_type.name() + " to " + to_type.name() + ".");
    }
}

SymbolType TypeRegistry::SYMBOL_TYPE;
EmptyListType TypeRegistry::EMPTY_LIST_TYPE;
BasicType TypeRegistry::NO_TYPE = BasicType(typeid(void), "<no type>");

TypeRegistry::TypeRegistry() {
    insert_basic_type<bool>("bool");
    insert_basic_type<string>("string");
    insert_basic_type<int>("int");
    insert_basic_type<double>("double");
}

template<typename T>
void TypeRegistry::insert_basic_type(const string &name) {
    type_index type = typeid(T);
    registered_types[type] = make_unique<BasicType>(type, name);
}

const FeatureType &TypeRegistry::create_feature_type(const CategoryPlugin &plugin) {
    type_index type = plugin.get_pointer_type();
    if (registered_types.count(type)) {
        ABORT("Creating the FeatureType '" + plugin.get_class_name()
              + "' but the type '" + registered_types[type]->name()
              + "' already exists and has the same type_index.");
    }
    unique_ptr<FeatureType> type_ptr = make_unique<FeatureType>(
        plugin.get_pointer_type(), plugin.get_category_name(),
        plugin.get_synopsis(), plugin.supports_variable_binding());
    const FeatureType &type_ref = *type_ptr;
    registered_types[type] = move(type_ptr);
    return type_ref;
}

const EnumType &TypeRegistry::create_enum_type(const EnumPlugin &plugin) {
    type_index type = plugin.get_type();
    const EnumInfo &values = plugin.get_enum_info();
    if (registered_types.count(type)) {
        ABORT("Creating the EnumType '" + plugin.get_class_name()
              + "' but the type '" + registered_types[type]->name()
              + "' already exists and has the same type_index.");
    }
    unique_ptr<EnumType> type_ptr = make_unique<EnumType>(type, values);
    const EnumType &type_ref = *type_ptr;
    registered_types[type] = move(type_ptr);
    return type_ref;
}

const ListType &TypeRegistry::create_list_type(const Type &element_type) {
    const Type *key = &element_type;
    if (!registered_list_types.count(key)) {
        registered_list_types.insert({key, make_unique<ListType>(element_type)});
    }
    return *registered_list_types[key];
}

const Type &TypeRegistry::get_nonlist_type(type_index type) const {
    if (!registered_types.count(type)) {
        return NO_TYPE;
    }
    return *registered_types.at(type);
}
}
