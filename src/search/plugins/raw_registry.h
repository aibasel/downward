#ifndef PLUGINS_RAW_REGISTRY_H
#define PLUGINS_RAW_REGISTRY_H

#include "registry_types.h"

#include <vector>

namespace plugins {
class RawRegistry {
    std::vector<const CategoryPlugin *> category_plugins;
    std::vector<const SubcategoryPlugin *> subcategory_plugins;
    std::vector<const EnumPlugin *> enum_plugins;
    std::vector<const Plugin *> plugins;

    RawRegistry() = default;
public:
    void insert_category_plugin(const CategoryPlugin &category_plugin);
    void insert_subcategory_plugin(const SubcategoryPlugin &subcategory_plugin);
    void insert_enum_plugin(const EnumPlugin &enum_plugin);
    void insert_plugin(const Plugin &plugin);

    const std::vector<const CategoryPlugin *> &get_category_plugins() const;
    const std::vector<const SubcategoryPlugin *> &
    get_subcategory_plugins() const;
    const std::vector<const EnumPlugin *> &get_enum_plugins() const;
    const std::vector<const Plugin *> &get_plugins() const;

    static RawRegistry *instance() {
        static RawRegistry instance_;
        return &instance_;
    }
};
}

#endif
