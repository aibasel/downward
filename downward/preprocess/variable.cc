#include "variable.h"

#include <cassert>
using namespace std;

Variable::Variable(istream &in) {
    in >> name >> range >> layer;
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
    cout << name << " [range " << range;
    if (level != -1)
        cout << "; level " << level;
    if (is_derived())
        cout << "; derived; layer: " << layer;
    cout << "]" << endl;
}
