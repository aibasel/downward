#include "string_utils.h"

#include "errors.h"

#include <algorithm>
#include <stdexcept>

using namespace std;

namespace options {
string sanitize_string(string s) {
    // Convert newlines to spaces.
    replace(s.begin(), s.end(), '\n', ' ');
    // Convert string to lower case.
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

int parse_int_arg(const string &name, const string &value) {
    try {
        return stoi(value);
    } catch (invalid_argument &) {
        throw ArgError("argument for " + name + " must be an integer");
    } catch (out_of_range &) {
        throw ArgError("argument for " + name + " is out of range");
    }
}


void ltrim(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch) {
                                   return !isspace(ch);
                               }));
}

void rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(), [](int ch) {
                        return !isspace(ch);
                    }).base(), s.end());
}

void trim(string &s) {
    ltrim(s);
    rtrim(s);
}


pair<string, string> split(const string &arg, const string &splitter) {
    int split_pos = arg.find(splitter);
    string lhs = arg.substr(0, split_pos);
    trim(lhs);
    string rhs = arg.substr(split_pos + 1);
    trim(rhs);
    return make_pair(lhs, rhs);
}
}
