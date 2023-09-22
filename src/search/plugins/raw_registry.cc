#include "raw_registry.h"

#include "plugin.h"
#include "registry.h"

#include "../utils/collections.h"
#include "../utils/strings.h"

#include <string>

using namespace std;

namespace plugins {
void RawRegistry::insert_category_plugin(const CategoryPlugin &category_plugin) {
    category_plugins.push_back(&category_plugin);
}

void RawRegistry::insert_subcategory_plugin(const SubcategoryPlugin &subcategory_plugin) {
    subcategory_plugins.push_back(&subcategory_plugin);
}

void RawRegistry::insert_enum_plugin(const EnumPlugin &enum_plugin) {
    enum_plugins.push_back(&enum_plugin);
}

void RawRegistry::insert_plugin(const Plugin &plugin) {
    plugins.push_back(&plugin);
}

FeatureTypes RawRegistry::collect_types(vector<string> &errors) const {
    FeatureTypes feature_types;
    unordered_map<type_index, vector<string>> type_to_names;

    for (const EnumPlugin *enum_plugin : enum_plugins) {
        vector<string> &names = type_to_names[enum_plugin->get_type()];
        if (names.empty()) {
            TypeRegistry::instance()->create_enum_type(*enum_plugin);
        }
        names.push_back("EnumPlugin(" + enum_plugin->get_class_name() + ")");
    }

    for (const CategoryPlugin *category_plugin : category_plugins) {
        vector<string> &names = type_to_names[category_plugin->get_pointer_type()];
        if (names.empty()) {
            const FeatureType &type =
                TypeRegistry::instance()->create_feature_type(*category_plugin);
            feature_types.push_back(&type);
        }
        names.push_back("CategoryPlugin(" + category_plugin->get_class_name() +
                        ", " + category_plugin->get_category_name() + ")");
    }

    // Check that each type index is only used once for either an enum or a category.
    for (const auto &pair : type_to_names) {
        const vector<string> &names = pair.second;
        if (names.size() > 1) {
            errors.push_back(
                "Multiple plugins are defined for the same type: " +
                utils::join(names, ", ") + "'.");
        }
    }
    return feature_types;
}

void RawRegistry::validate_category_names(vector<string> &errors) const {
    unordered_map<string, vector<string>> category_name_to_class_names;
    unordered_map<string, vector<string>> class_name_to_category_names;
    for (const CategoryPlugin *category_plugin : category_plugins) {
        string class_name = category_plugin->get_class_name();
        string category_name = category_plugin->get_category_name();
        category_name_to_class_names[category_name].push_back(class_name);
        class_name_to_category_names[class_name].push_back(category_name);
    }

    for (const auto &pair : category_name_to_class_names) {
        const string &category_name = pair.first;
        const vector<string> &class_names = pair.second;
        if (class_names.size() > 1) {
            errors.push_back(
                "Multiple CategoryPlugins have the name '" +
                category_name + "': " + utils::join(class_names, ", ") + ".");
        }
    }
    for (const auto &pair : class_name_to_category_names) {
        const string &class_name = pair.first;
        const vector<string> &category_names = pair.second;
        if (category_names.size() > 1) {
            errors.push_back(
                "Multiple CategoryPlugins are defined for the class '" + class_name +
                "': " + utils::join(category_names, ", ") + ".");
        }
    }
}

SubcategoryPlugins RawRegistry::collect_subcategory_plugins(
    vector<string> &errors) const {
    SubcategoryPlugins subcategory_plugin_map;
    unordered_map<string, int> occurrences;

    for (const SubcategoryPlugin *subcategory_plugin : subcategory_plugins) {
        ++occurrences[subcategory_plugin->get_subcategory_name()];
        subcategory_plugin_map.emplace(subcategory_plugin->get_subcategory_name(),
                                       subcategory_plugin);
    }

    for (auto &item : occurrences) {
        string subcategory = item.first;
        int occurrence = item.second;
        if (occurrence > 1) {
            errors.push_back(
                "The SubcategoryPlugin '" + subcategory + "' is defined " +
                to_string(occurrence) + " times.");
        }
    }
    return subcategory_plugin_map;
}

Features RawRegistry::collect_features(
    const SubcategoryPlugins &subcategory_plugins, vector<string> &errors) const {
    Features features;
    unordered_map<string, int> feature_key_occurrences;
    for (const Plugin *plugin : plugins) {
        shared_ptr<Feature> feature = plugin->create_feature();
        string feature_key = feature->get_key();
        feature_key_occurrences[feature_key]++;
        features[feature_key] = move(feature);
    }

    // Check that feature_keys are unique
    for (const auto &pair : feature_key_occurrences) {
        const string &feature_key = pair.first;
        int occurrences = pair.second;
        if (occurrences > 1) {
            errors.push_back(
                to_string(occurrences) + " Features are defined for the key '" +
                feature_key + "'.");
        }
    }

    // Check that all subcategories used in features are defined
    for (const auto &item : features) {
        const string &feature_key = item.first;
        const Feature &feature = *item.second;
        string subcategory = feature.get_subcategory();

        if (!subcategory.empty() && !subcategory_plugins.count(subcategory)) {
            const Type &type = feature.get_type();
            errors.push_back(
                "Missing SubcategoryPlugin '" + subcategory + "' for Plugin '" +
                feature_key + "' of type " + type.name());
        }
    }

    // Check that all types used in features are defined
    unordered_set<type_index> missing_types;
    for (const auto &item : features) {
        const string &feature_key = item.first;
        const Feature &feature = *item.second;

        const Type &type = feature.get_type();
        if (type == TypeRegistry::NO_TYPE) {
            errors.push_back(
                "Missing Plugin for type of feature '" + feature_key + "'.");
        }

        unordered_map<string, int> parameter_occurrences;
        for (const ArgumentInfo &arg_info : feature.get_arguments()) {
            if (arg_info.type == TypeRegistry::NO_TYPE) {
                errors.push_back(
                    "Missing Plugin for type of parameter '" + arg_info.key
                    + "' of feature '" + feature_key + "'.");
            }
            ++parameter_occurrences[arg_info.key];
        }
        // Check that parameters are unique
        for (const auto &pair : parameter_occurrences) {
            const string &parameter = pair.first;
            int parameter_occurrence = pair.second;
            if (parameter_occurrence > 1) {
                errors.push_back(
                    "The parameter '" + parameter + "' in '" + feature_key + "' is defined " +
                    to_string(parameter_occurrence) + " times.");
            }
        }
    }

    return features;
}

Registry RawRegistry::construct_registry() const {
    vector<string> errors;
    FeatureTypes feature_types = collect_types(errors);
    validate_category_names(errors);
    SubcategoryPlugins subcategory_plugins = collect_subcategory_plugins(errors);
    Features features = collect_features(subcategory_plugins, errors);

    if (!errors.empty()) {
        sort(errors.begin(), errors.end());
        cerr << "Internal registry error(s):" << endl;
        for (const string &error : errors) {
            cerr << error << endl;
        }
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    return Registry(
        move(feature_types),
        move(subcategory_plugins),
        move(features));
}
}
