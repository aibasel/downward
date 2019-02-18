#include "strings.h"

#include <algorithm>
#include <iostream>

using namespace std;

namespace utils {
StringOperationError::StringOperationError(const string &msg)
    : msg(msg) {
}

void StringOperationError::print() const {
    cerr << msg << endl;
}


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

bool startswith(const string &s, const string &prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}

pair<string, string> split(const string &s, const string &separator) {
    int split_pos = s.find(separator);
    if (split_pos == -1) {
        throw StringOperationError("separator not found");
    }
    string lhs = s.substr(0, split_pos);
    string rhs = s.substr(split_pos + 1);
    return make_pair(lhs, rhs);
}
}
