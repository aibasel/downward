#include "options.h"

using namespace std;

namespace plugins {
Options::Options() {
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
