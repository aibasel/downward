#include "options.h"

using namespace std;

namespace options {
Options::Options(bool help_mode)
    : unparsed_config("<missing>"),
      help_mode(help_mode) {
}

bool Options::contains(const string &key) const {
    return storage.find(key) != storage.end();
}

const string &Options::get_unparsed_config() const {
    return unparsed_config;
}

void Options::set_unparsed_config(const string &config) {
    unparsed_config = config;
}
}
