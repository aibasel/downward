#include "strings.h"

#include <algorithm>

using namespace std;

namespace utils {
StringOperationError::StringOperationError(const string &msg)
    : msg(msg) {
}

ostream &operator<<(ostream &out, const StringOperationError &err) {
    return out << "string operation error: " << err.msg;
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

bool startswith(const string &str, const string &prefix) {
    return str.compare(0, prefix.size(), prefix) == 0;
}

pair<string, string> split(const string &str, const string &separator) {
    int split_pos = str.find(separator);
    if (split_pos == -1) {
        throw StringOperationError("Separator not found.");
    }
    string lhs = str.substr(0, split_pos);
    string rhs = str.substr(split_pos + 1);
    return make_pair(lhs, rhs);
}
}
