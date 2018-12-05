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


pair<string, string> split(const string &arg, const string &splitter) {
    int split_pos = arg.find(splitter);
    string lhs = arg.substr(0, split_pos);
    strip(lhs);
    string rhs = arg.substr(split_pos + 1);
    strip(rhs);
    return make_pair(lhs, rhs);
}
}
