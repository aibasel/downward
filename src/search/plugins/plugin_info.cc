#include "plugin_info.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace plugins {
const string ArgumentInfo::NO_DEFAULT = "<none>";

ArgumentInfo::ArgumentInfo(
    const string &key,
    const string &help,
    const Type &type,
    const string &default_value,
    const Bounds &bounds,
    bool lazy_construction)
    : key(key),
      help(help),
      type(type),
      default_value(default_value),
      bounds(bounds),
      lazy_construction(lazy_construction) {
}

bool ArgumentInfo::is_optional() const {
    return !default_value.empty();
}

bool ArgumentInfo::has_default() const {
    return is_optional() && default_value != NO_DEFAULT;
}

PropertyInfo::PropertyInfo(
    const string &property,
    const string &description)
    : property(property),
      description(description) {
}

NoteInfo::NoteInfo(
    const string &name,
    const string &description,
    bool long_text)
    : name(name),
      description(description),
      long_text(long_text) {
}

LanguageSupportInfo::LanguageSupportInfo(
    const string &feature,
    const string &description)
    : feature(feature),
      description(description) {
}
}
