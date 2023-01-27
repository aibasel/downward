#include "plugin.h"

#include "../utils/strings.h"

using namespace std;

namespace plugins {
Feature::Feature(const Type &type, const string &key)
    : type(type), key(utils::tolower(key)) {
}

void Feature::document_subcategory(const string &subcategory) {
    this->subcategory = subcategory;
}

void Feature::document_title(const string &title) {
    this->title = title;
}

void Feature::document_synopsis(const string &note) {
    synopsis = note;
}

void Feature::document_property(
    const string &property, const string &note) {
    properties.emplace_back(property, note);
}

void Feature::document_language_support(
    const string &feature, const string &note) {
    language_support.emplace_back(feature, note);
}

void Feature::document_note(
    const string &name, const string &note, bool long_text) {
    notes.emplace_back(name, note, long_text);
}

const Type &Feature::get_type() const {
    return type;
}

string Feature::get_key() const {
    return key;
}

string Feature::get_title() const {
    return title;
}

string Feature::get_synopsis() const {
    return synopsis;
}

string Feature::get_subcategory() const {
    return subcategory;
}

const vector<ArgumentInfo> &Feature::get_arguments() const {
    return arguments;
}

const vector<PropertyInfo> &Feature::get_properties() const {
    return properties;
}

const vector<LanguageSupportInfo> &Feature::get_language_support() const {
    return language_support;
}

const vector<NoteInfo> &Feature::get_notes() const {
    return notes;
}

Plugin::Plugin() {
    RawRegistry::instance()->insert_plugin(*this);
}

CategoryPlugin::CategoryPlugin(
    type_index pointer_type, const string &class_name, const string &category_name)
    : pointer_type(pointer_type), class_name(class_name),
      category_name(category_name), can_be_bound_to_variable(false) {
    RawRegistry::instance()->insert_category_plugin(*this);
}

type_index CategoryPlugin::get_pointer_type() const {
    return pointer_type;
}

string CategoryPlugin::get_category_name() const {
    return category_name;
}

string CategoryPlugin::get_class_name() const {
    return class_name;
}

string CategoryPlugin::get_synopsis() const {
    return synopsis;
}

bool CategoryPlugin::supports_variable_binding() const {
    return can_be_bound_to_variable;
}

void CategoryPlugin::document_synopsis(const string &synopsis) {
    this->synopsis = synopsis;
}

void CategoryPlugin::allow_variable_binding() {
    can_be_bound_to_variable = true;
}

SubcategoryPlugin::SubcategoryPlugin(const string &subcategory)
    : subcategory_name(subcategory) {
    RawRegistry::instance()->insert_subcategory_plugin(*this);
}

void SubcategoryPlugin::document_title(const string &title) {
    this->title = title;
}

void SubcategoryPlugin::document_synopsis(const string &synopsis) {
    this->synopsis = synopsis;
}

string SubcategoryPlugin::get_subcategory_name() const {
    return subcategory_name;
}

string SubcategoryPlugin::get_title() const {
    return title;
}

string SubcategoryPlugin::get_synopsis() const {
    return synopsis;
}

EnumPlugin::EnumPlugin(type_index type, const string &class_name,
                       initializer_list<pair<string, string>> enum_values)
    : type(type), class_name(class_name), enum_info(enum_values) {
    RawRegistry::instance()->insert_enum_plugin(*this);
}

type_index EnumPlugin::get_type() const {
    return type;
}

string EnumPlugin::get_class_name() const {
    return class_name;
}
const EnumInfo &EnumPlugin::get_enum_info() const {
    return enum_info;
}
}
