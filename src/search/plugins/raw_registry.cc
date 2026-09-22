#include "raw_registry.h"

#include "plugin.h"
#include "registry.h"

#include "../utils/collections.h"
#include "../utils/strings.h"

#include <string>

using namespace std;

namespace plugins {
void RawRegistry::insert_category_plugin(
    const CategoryPlugin &category_plugin) {
    category_plugins.push_back(&category_plugin);
}

void RawRegistry::insert_subcategory_plugin(
    const SubcategoryPlugin &subcategory_plugin) {
    subcategory_plugins.push_back(&subcategory_plugin);
}

void RawRegistry::insert_enum_plugin(const EnumPlugin &enum_plugin) {
    enum_plugins.push_back(&enum_plugin);
}

void RawRegistry::insert_plugin(const Plugin &plugin) {
    plugins.push_back(&plugin);
}

const vector<const CategoryPlugin *> &
RawRegistry::get_category_plugins() const {
    return category_plugins;
}

const vector<const SubcategoryPlugin *> &
RawRegistry::get_subcategory_plugins() const {
    return subcategory_plugins;
}

const vector<const EnumPlugin *> &RawRegistry::get_enum_plugins() const {
    return enum_plugins;
}

const vector<const Plugin *> &RawRegistry::get_plugins() const {
    return plugins;
}

}
