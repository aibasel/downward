#include "options.h"

using namespace std;

namespace options {
Options::Options(bool help_mode)
    : unparsed_config("<missing>"),
      help_mode(help_mode) {
}

int Options::get_enum(const std::string &key) const {
    return get<int>(key);
}

bool Options::contains(const std::string &key) const {
    return storage.find(key) != storage.end();
}

const std::string &Options::get_unparsed_config() const {
    return unparsed_config;
}

void Options::set_unparsed_config(const std::string &config) {
    unparsed_config = config;
}
}
