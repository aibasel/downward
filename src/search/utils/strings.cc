#include "strings.h"

#include <algorithm>

using namespace std;

namespace utils {
void lstrip(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch) {
                                   return !isspace(ch);
                               }));
}

void rstrip(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(), [](int ch) {
                        return !isspace(ch);
                    }).base(), s.end());
}

void strip(string &s) {
    lstrip(s);
    rstrip(s);
}

bool startswith(const string &str, const string &prefix) {
    return !str.compare(0, prefix.size(), prefix);
}

pair<string, string> split(const string &str, const string &separator,
                           bool strip) {
    int split_pos = str.find(separator);
    string lhs = str.substr(0, split_pos);
    string rhs = str.substr(split_pos + 1);
    if (strip) {
        utils::strip(lhs);
        utils::strip(rhs);
    }
    return make_pair(lhs, rhs);
}
}
