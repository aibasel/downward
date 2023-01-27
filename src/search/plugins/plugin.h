#ifndef PLUGINS_PLUGIN_H
#define PLUGINS_PLUGIN_H

#include "any.h"
#include "options.h"
#include "plugin_info.h"
#include "raw_registry.h"

#include "../utils/strings.h"
#include "../utils/system.h"

#include <string>
#include <typeindex>
#include <type_traits>
#include <vector>

namespace utils {
class Context;
}

namespace plugins {
class Feature {
    const Type &type;
    std::string key;
    std::string title;
    std::string synopsis;
    std::string subcategory;
    std::vector<ArgumentInfo> arguments;
    std::vector<PropertyInfo> properties;
    std::vector<LanguageSupportInfo> language_support;
    std::vector<NoteInfo> notes;
public:
    Feature(const Type &type, const std::string &key);
    virtual ~Feature() = default;
    Feature(const Feature &) = delete;

    virtual Any construct(const Options &opts, const utils::Context &context) const = 0;

    /* Add option with default value. Use def_val=ArgumentInfo::NO_DEFAULT for
       optional parameters without default values. */
    template<typename T>
    void add_option(
        const std::string &key,
        const std::string &help = "",
        const std::string &default_value = "",
        const Bounds &bounds = Bounds::unlimited(),
        bool lazy_construction = false);

    template<typename T>
    void add_list_option(
        const std::string &key,
        const std::string &help = "",
        const std::string &default_value = "",
        bool lazy_construction = false);

    void document_subcategory(const std::string &subcategory);
    void document_title(const std::string &title);
    void document_synopsis(const std::string &note);
    void document_property(
        const std::string &property, const std::string &note);
    void document_language_support(
        const std::string &feature, const std::string &note);
    void document_note(
        const std::string &title, const std::string &note, bool long_text = false);

    const Type &get_type() const;
    std::string get_key() const;
    std::string get_title() const;
    std::string get_synopsis() const;
    std::string get_subcategory() const;
    const std::vector<ArgumentInfo> &get_arguments() const;
    const std::vector<PropertyInfo> &get_properties() const;
    const std::vector<LanguageSupportInfo> &get_language_support() const;
    const std::vector<NoteInfo> &get_notes() const;
};


template<typename Constructed>
class FeatureWithDefault : public Feature {
protected:
    using Feature::Feature;
    virtual std::shared_ptr<Constructed> create_component(
        const Options &options, const utils::Context &) const {
        return std::make_shared<Constructed>(options);
    }
};

template<typename Constructed>
class FeatureWithoutDefault : public Feature {
protected:
    using Feature::Feature;
    virtual std::shared_ptr<Constructed> create_component(
        const Options &, const utils::Context &) const = 0;
};

template<typename Constructed>
using FeatureAuto = typename std::conditional<
    std::is_constructible<Constructed, const Options &>::value,
    FeatureWithDefault<Constructed>,
    FeatureWithoutDefault<Constructed>>::type;

template<typename Base, typename Constructed>
class TypedFeature : public FeatureAuto<Constructed> {
    using BasePtr = std::shared_ptr<Base>;
    static_assert(std::is_base_of<Base, Constructed>::value,
                  "Constructed must derive from Base");
public:
    TypedFeature(const std::string &key)
        : FeatureAuto<Constructed>(TypeRegistry::instance()->get_type<BasePtr>(), key) {
    }

    Any construct(const Options &options, const utils::Context &context) const override {
        std::shared_ptr<Base> ptr = this->create_component(options, context);
        return Any(ptr);
    }
};

class Plugin {
public:
    Plugin();
    virtual ~Plugin() = default;
    Plugin(const Plugin &) = delete;
    virtual std::shared_ptr<Feature> create_feature() const = 0;
};

template<typename T>
class FeaturePlugin : public Plugin {
public:
    FeaturePlugin() : Plugin() {
    }
    virtual std::shared_ptr<Feature> create_feature() const override {
        return std::make_shared<T>();
    }
};

/*
  The CategoryPlugin class contains meta-information for a given
  category of feature (e.g. "SearchEngine" or "MergeStrategyFactory").
*/
class CategoryPlugin {
    std::type_index pointer_type;
    std::string class_name;

    /*
      The category name should be "user-friendly". It is for example used
      as the name of the wiki page that documents this feature type.
      It follows wiki conventions (e.g. "Heuristic", "SearchEngine",
      "ShrinkStrategy").
    */
    std::string category_name;

    /*
      General documentation for the feature type. This is included at
      the top of the wiki page for this feature type.
    */
    std::string synopsis;

    /*
      TODO: Currently, we do not support variable binding of all categories, so
      variables can only be used for categories explicitly marked. This might
      change once we fix the component interaction (issue559). If all feature
      types can be bound to variables, we can probably get rid of this flag and
      related code in CategoryPlugin, TypedCategoryPlugin, RawRegistry,
      Registry, Parser, ...
    */
    bool can_be_bound_to_variable;
public:
    CategoryPlugin(
        std::type_index pointer_type,
        const std::string &class_name,
        const std::string &category_name);
    virtual ~CategoryPlugin() = default;
    CategoryPlugin(const CategoryPlugin &) = delete;

    void document_synopsis(const std::string &synopsis);
    void allow_variable_binding();

    std::type_index get_pointer_type() const;
    std::string get_category_name() const;
    std::string get_class_name() const;
    std::string get_synopsis() const;
    bool supports_variable_binding() const;
};

template<typename T>
class TypedCategoryPlugin : public CategoryPlugin {
public:
    TypedCategoryPlugin(const std::string &category_name)
        : CategoryPlugin(typeid(std::shared_ptr<T>),
                         utils::get_type_name<std::shared_ptr<T>>(),
                         category_name) {
    }
};

class SubcategoryPlugin {
    std::string subcategory_name;
    std::string title;
    std::string synopsis;
public:
    SubcategoryPlugin(const std::string &subcategory);

    void document_title(const std::string &title);
    void document_synopsis(const std::string &synopsis);

    std::string get_subcategory_name() const;
    std::string get_title() const;
    std::string get_synopsis() const;
};

class EnumPlugin {
    std::type_index type;
    std::string class_name;
    EnumInfo enum_info;
public:
    EnumPlugin(std::type_index type, const std::string &class_name,
               std::initializer_list<std::pair<std::string, std::string>> enum_values);

    std::type_index get_type() const;
    std::string get_class_name() const;
    const EnumInfo &get_enum_info() const;
};

template<typename T>
class TypedEnumPlugin : public EnumPlugin {
public:
    TypedEnumPlugin(std::initializer_list<std::pair<std::string, std::string>> enum_values)
        : EnumPlugin(typeid(T), utils::get_type_name<std::shared_ptr<T>>(), enum_values) {
    }
};


template<typename T>
void Feature::add_option(
    const std::string &key,
    const std::string &help,
    const std::string &default_value,
    const Bounds &bounds,
    bool lazy_construction) {
    arguments.emplace_back(key, help, TypeRegistry::instance()->get_type<T>(),
                           default_value, bounds, lazy_construction);
}

template<typename T>
void Feature::add_list_option(
    const std::string &key,
    const std::string &help,
    const std::string &default_value,
    bool lazy_construction) {
    add_option<std::vector<T>>(key, help, default_value, Bounds::unlimited(),
                               lazy_construction);
}
}

#endif
