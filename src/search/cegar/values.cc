#include "values.h"

#include <vector>

using namespace std;

namespace cegar_heuristic {


void Values::add(int var, int value) {
    values[var].set(value);
}

void Values::remove(int var, int value) {
    values[var].reset(value);
}

void Values::add_all(int var) {
    values[var].set();
}

void Values::clear(int var) {
    values[var].clear();
}

bool Values::test(int var, int value) const {
    return values[var].test(value);
}

int Values::count(int var) const {
    return values[var].count();
}

//int Values::get_var(int pos) const {
//
//}

bool Values::intersects(int var, const Values &other) const {
    return values[var].intersects(other.values[var]);
}

}
