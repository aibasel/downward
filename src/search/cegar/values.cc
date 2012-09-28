#include "values.h"

#include <vector>

using namespace std;

namespace cegar_heuristic {


void Values::set_value(int var, int value, bool val) {
    values[var].set(value, val);
}

void Values::set_var(int var) {
    values[var].set();
}

void Values::reset_var(int var) {
    values[var].reset();
}

bool Values::test(int var, int value) const {
    return values[var].test(value);
}

int Values::count(int var) const {
    return values[var].count();
}

void Values::clear(int var) {
    values[var].clear();
}

//int Values::get_var(int pos) const {
//
//}

bool Values::intersects(int var, const Values &other) const {
    return values[var].intersects(other.values[var]);
}

}
