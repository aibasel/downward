#include "values.h"

#include <sstream>
#include <vector>

using namespace std;

namespace cegar_heuristic {

Values::Values() {
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        Domain domain(g_variable_domain[var]);
        domain.set();
        values.push_back(domain);
    }
}

void Values::add(int var, int value) {
    values[var].set(value);
}

void Values::remove(int var, int value) {
    values[var].reset(value);
}

void Values::set(int var, int value) {
    remove_all(var);
    add(var, value);
}

void Values::add_all(int var) {
    values[var].set();
}

void Values::remove_all(int var) {
    values[var].reset();
}

bool Values::test(int var, int value) const {
    return values[var].test(value);
}

int Values::count(int var) const {
    return values[var].count();
}

bool Values::intersects(int var, const Values &other) const {
    return values[var].intersects(other.values[var]);
}

bool Values::all_vars_intersect(const Values &other, const vector<bool> &checked) const {
    for (int var = 0; var < values.size(); ++var) {
        if (!checked[var] && !values[var].intersects(other.values[var]))
            return false;
    }
    return true;
}

bool Values::abstracts(const Values &other) const {
    for (int i = 0; i < values.size(); ++i) {
        if (!other.values[i].is_subset_of(values[i]))
            return false;
    }
    return true;
}

void Values::get_unmet_conditions(const Values &other, Conditions *conditions) const {
    // Get all set intersections of the possible values here with the possible
    // values in "desired".
    for (int i = 0; i < g_variable_domain.size(); ++i) {
        Domain intersection = values[i] & other.values[i];
        if (intersection.count() < values[i].count()) {
            int next_value = intersection.find_first();
            while (next_value != Domain::npos) {
                // The variable's value matters for determining the resulting state.
                conditions->push_back(pair<int, int>(i, next_value));
                next_value = intersection.find_next(next_value);
            }
        }
    }
}

string Values::str() const {
    ostringstream oss;
    string sep = "";
    for (int i = 0; i < values.size(); ++i) {
        const Domain &vals = values[i];
        if (vals.count() != g_variable_domain[i]) {
            oss << sep << i << "=" << domain_to_string(vals);
            sep = ",";
        }
    }
    return oss.str();
}

}
