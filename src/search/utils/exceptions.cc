#include "exceptions.h"

#include <iostream>

using namespace std;

namespace utils {
Exception::Exception(const string &msg) : msg(msg) {
}

string Exception::get_message() const {
    return msg;
}

void Exception::print() const {
    cerr << msg << endl;
}
}
