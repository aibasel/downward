#include "registry.h"

#include "plugin.h"

#include "../utils/system.h"

#include <algorithm>

using namespace std;

namespace plugins {
static void validate_category_names(const RawRegistry &raw_registry, vector<string> &errors) {
    unordered_map<string, vector<string>> category_name_to_class_names;
    unordered_map<string, vector<string>> class_name_to_category_names;
    for (const CategoryPlugin *category_plugin : raw_registry.get_category_plugins()) {
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
                "Multiple CategoryPlugins have the name '" + category_name +
                "': " + utils::join(class_names, ", ") + ".");
        }
    }
    for (const auto &pair : class_name_to_category_names) {
        const string &class_name = pair.first;
        const vector<string> &category_names = pair.second;
        if (category_names.size() > 1) {
            errors.push_back(
                "Multiple CategoryPlugins are defined for the class '" +
                class_name + "': " + utils::join(category_names, ", ") + ".");
        }
    }
}

static SubcategoryPlugins collect_subcategory_plugins(
    const RawRegistry &raw_registry,
    vector<string> &errors) {
    SubcategoryPlugins subcategory_plugin_map;
    unordered_map<string, int> occurrences;

    for (const SubcategoryPlugin *subcategory_plugin : raw_registry.get_subcategory_plugins()) {
        ++occurrences[subcategory_plugin->get_subcategory_name()];
        subcategory_plugin_map.emplace(
            subcategory_plugin->get_subcategory_name(), subcategory_plugin);
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

static Features collect_features(
    const RawRegistry &raw_registry,
    const SubcategoryPlugins &subcategory_plugins,
    vector<string> &errors) {
    Features features;
    unordered_map<string, int> feature_key_occurrences;
    for (const Plugin *plugin : raw_registry.get_plugins()) {
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
                    "Missing Plugin for type of parameter '" + arg_info.key +
                    "' of feature '" + feature_key + "'.");
            }
            ++parameter_occurrences[arg_info.key];
        }
        // Check that parameters are unique
        for (const auto &pair : parameter_occurrences) {
            const string &parameter = pair.first;
            int parameter_occurrence = pair.second;
            if (parameter_occurrence > 1) {
                errors.push_back(
                    "The parameter '" + parameter + "' in '" + feature_key +
                    "' is defined " + to_string(parameter_occurrence) +
                    " times.");
            }
        }
    }

    return features;
}

Registry::Registry() {
    const RawRegistry &raw_registry = *RawRegistry::instance();
    vector<string> errors;
    feature_types = TypeRegistry::instance()->get_feature_types();
    validate_category_names(raw_registry, errors);
    subcategory_plugins =
        collect_subcategory_plugins(raw_registry, errors);
    features = collect_features(raw_registry, subcategory_plugins, errors);

    if (!errors.empty()) {
        sort(errors.begin(), errors.end());
        cerr << "Internal registry error(s):" << endl;
        for (const string &error : errors) {
            cerr << error << endl;
        }
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

shared_ptr<const Feature> Registry::get_feature(const string &name) const {
    if (!has_feature(name)) {
        ABORT("could not find a feature named '" + name + "' in the registry");
    }
    return features.at(name);
}

const SubcategoryPlugin &Registry::get_subcategory_plugin(
    const string &subcategory) const {
    if (!subcategory_plugins.count(subcategory)) {
        ABORT(
            "attempt to retrieve non-existing group info from registry: " +
            string(subcategory));
    }
    return *subcategory_plugins.at(subcategory);
}

bool Registry::has_feature(const string &name) const {
    return features.count(name);
}

const FeatureTypes &Registry::get_feature_types() const {
    return feature_types;
}

vector<const SubcategoryPlugin *> Registry::get_subcategory_plugins() const {
    vector<const SubcategoryPlugin *> result;
    for (const auto &pair : subcategory_plugins) {
        result.push_back(pair.second);
    }
    return result;
}

vector<shared_ptr<const Feature>> Registry::get_features() const {
    vector<shared_ptr<const Feature>> result;
    for (const auto &pair : features) {
        result.push_back(pair.second);
    }
    return result;
}

const Registry *Registry::instance() {
    static Registry instance_;
    return &instance_;
}

}
