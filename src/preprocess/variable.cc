#include "variable.h"

#include "helper_functions.h"

#include <cassert>
using namespace std;

Variable::Variable(istream &in) {
    check_magic(in, "begin_variable");
    in >> ws >> name >> layer >> range >> ws;
    values.resize(range);
    for (size_t i = 0; i < range; ++i)
        getline(in, values[i]);
    check_magic(in, "end_variable");
    level = -1;
    necessary = false;
}

void Variable::set_level(int theLevel) {
    assert(level == -1);
    level = theLevel;
}

int Variable::get_level() const {
    return level;
}

void Variable::set_necessary() {
    assert(necessary == false);
    necessary = true;
}

int Variable::get_range() const {
    return range;
}

string Variable::get_name() const {
    return name;
}

bool Variable::is_necessary() const {
    return necessary;
}

void Variable::dump() const {
    // TODO: Dump values (and other information that might be missing?)
    //       or get rid of this if it's no longer needed.
    cout << name << " [range " << range;
    if (level != -1)
        cout << "; level " << level;
    if (is_derived())
        cout << "; derived; layer: " << layer;
    cout << "]" << endl;
}
