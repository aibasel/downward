#include "strings.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>

using namespace std;

namespace utils {
string escape(const string &s) {
    /*
      Escape any occurrences of \ with \\, occurrences of " with \" and
      newlines with \n.
    */
    string result;
    result.reserve(s.length());
    for (char c : s) {
        if (c == '\\') {
            result += "\\\\";
        } else if (c == '"') {
            result += "\\\"";
        } else if (c == '\n') {
            result += "\\n";
        } else {
            result += c;
        }
    }
    return result;
}

string unescape(const string &s) {
    /*
      On sequences created with escape(), this will restore the original string.
      However, no syntax checking is done. Escaped symbols other than the ones
      added by escape() will just ignore the escaping \ (e.g., \t is treated
      as t, not as a tab). Strings ending in \ will not produce an error.
    */
    string result;
    result.reserve(s.length());
    bool escaped = false;
    for (char c : s) {
        if (escaped) {
            escaped = false;
            if (c == 'n') {
                result += '\n';
            } else {
                result += c;
            }
        } else if (c == '\\') {
            escaped = true;
        } else {
            result += c;
        }
    }
    return result;
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

string tolower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

vector<string> split(const string &s, const string &separator, int max_splits) {
    assert(max_splits >= -1);
    vector<string> sections;
    int curr_pos = 0;
    int next_pos = s.find(separator);
    while (max_splits != 0 && next_pos != -1) {
        sections.push_back(s.substr(curr_pos, next_pos - curr_pos));
        curr_pos = next_pos + 1;
        next_pos = s.find(separator, curr_pos);
        --max_splits;
    }
    sections.push_back(s.substr(curr_pos, s.size() - curr_pos));
    return sections;
}

bool is_alpha_numeric(const string &s) {
    auto it = find_if(s.begin(), s.end(), [](char const &c) {
                          return !isalnum(c);
                      });
    return it == s.end();
}
}
