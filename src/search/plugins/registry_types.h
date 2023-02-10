#ifndef PLUGINS_REGISTRY_TYPES_H
#define PLUGINS_REGISTRY_TYPES_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace plugins {
class CategoryPlugin;
class EnumPlugin;
class Feature;
class FeatureType;
class Plugin;
class Registry;
class SubcategoryPlugin;

using FeatureTypes = std::vector<const FeatureType *>;
using EnumInfo = std::vector<std::pair<std::string, std::string>>;
using Features = std::unordered_map<std::string, std::shared_ptr<Feature>>;
using SubcategoryPlugins = std::unordered_map<std::string, const SubcategoryPlugin *>;
}
#endif
