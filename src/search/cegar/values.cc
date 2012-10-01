#include "values.h"

#include <sstream>
#include <vector>

using namespace std;

namespace cegar_heuristic {

int Values::facts = -1;
vector<int> Values::borders;
vector<Bitset> Values::masks;

Values::Values() {
    if (facts == -1)
        initialize_static_members();

    assert(facts >= 0);
    values = Bitset(facts);
    values.set();
}

void Values::initialize_static_members() {
    assert(masks.empty());
    assert(borders.empty());
    facts = 0;
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        borders.push_back(facts);
        facts += g_variable_domain[var];
    }
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        // ---0000 -> 1110000
        Bitset mask(borders[var]);
        mask.resize(borders[var] + g_variable_domain[var], true);
        masks.push_back(mask);
    }
}

void Values::add(int var, int value) {
    values.set(pos(var, value));
}

void Values::remove(int var, int value) {
    values.reset(pos(var, value));
}

void Values::set(int var, int value) {
    remove_all(var);
    add(var, value);
}

void Values::add_all(int var) {
    for (int pos = borders[var]; pos < borders[var] + g_variable_domain[var]; ++pos)
        values.set(pos);
}

void Values::remove_all(int var) {
    for (int pos = borders[var]; pos < borders[var] + g_variable_domain[var]; ++pos)
        values.reset(pos);
}

bool Values::test(int var, int value) const {
    return values.test(pos(var, value));
}

int Values::count(int var) const {
    int num_values = 0;
    for (int pos = borders[var]; pos < borders[var] + g_variable_domain[var]; ++pos) {
        if (values.test(pos))
            ++num_values;
    }
    return num_values;
}

bool Values::all_vars_intersect(const Values &other, const vector<bool> &checked) const {
    Bitset intersection = values & other.values;
    for (int var = 0; var < borders.size(); ++var) {
        if (checked[var])
            continue;
        bool var_intersects = false;
        for (int pos = borders[var]; pos < borders[var] + g_variable_domain[var]; ++pos) {
            if (intersection.test(pos)) {
                var_intersects = true;
                break;
            }
        }
        if (!var_intersects)
            return false;
    }
    return true;
}

bool Values::abstracts(const Values &other) const {
    return other.values.is_subset_of(values);
}

void Values::get_unmet_conditions(const Values &other, Conditions *conditions) const {
    // Get all set intersections of the possible values here with the possible
    // values in "desired".
    Bitset intersection = values & other.values;
    for (int var = 0; var < borders.size(); ++var) {
        int next_border = borders[var] + g_variable_domain[var];
        vector<int> facts;
        int pos = (var == 0) ? intersection.find_first() : intersection.find_next(borders[var] - 1);
        while (pos != Bitset::npos && pos < next_border) {
            // The variable's value matters for determining the resulting state.
            facts.push_back(pos - borders[var]);
            pos = intersection.find_next(pos);
        }
        assert(!facts.empty());
        if (facts.size() < count(var)) {
            for (int i = 0; i < facts.size(); ++i)
                conditions->push_back(pair<int, int>(var, facts[i]));
        }
    }
}

string Values::str() const {
    ostringstream oss;
    string sep = "";
    for (int var = 0; var < borders.size(); ++var) {
        int next_border = borders[var] + g_variable_domain[var];
        vector<int> facts;
        int pos = (var == 0) ? values.find_first() : values.find_next(borders[var] - 1);
        while (pos != Bitset::npos && pos < next_border) {
            // The variable's value matters for determining the resulting state.
            facts.push_back(pos - borders[var]);
            pos = values.find_next(pos);
        }
        assert(!facts.empty());
        if (facts.size() < g_variable_domain[var]) {
            oss << sep << var << "={";
            sep = ",";
            string value_sep = "";
            for (int i = 0; i < facts.size(); ++i) {
                oss << value_sep << facts[i];
                value_sep = ",";
            }
            oss << "}";
        }
    }
    return oss.str();
}

}
