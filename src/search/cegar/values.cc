#include "values.h"

#include <sstream>

using namespace std;

namespace cegar_heuristic {
int Values::facts = -1;
vector<int> Values::borders;
vector<Bitset> Values::masks;

Values::Values() {
    assert(facts >= 0 && "Static members have not been initialized.");
    values = Bitset(facts);
    values.set();
}

void Values::initialize_static_members() {
    masks.clear();
    borders.clear();
    facts = 0;
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        borders.push_back(facts);
        facts += g_variable_domain[var];
    }
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        // -----0000 -> --1110000 --> 001110000
        Bitset mask(borders[var]);
        mask.resize(borders[var] + g_variable_domain[var], true);
        mask.resize(facts, false);
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
    values |= masks[var];
}

void Values::remove_all(int var) {
    values &= ~masks[var];
}

bool Values::test(int var, int value) const {
    return values.test(pos(var, value));
}

int Values::count(int var) const {
    // Profiling showed that an explicit loop is faster (~3 times) than
    // doing: return (values & masks[var]).count();
    int num_values = 0;
    for (int pos = borders[var]; pos < borders[var] + g_variable_domain[var]; ++pos) {
        if (values.test(pos))
            ++num_values;
    }
    return num_values;
}

bool Values::domains_intersect(const Values &other, int var) {
    // Using test() directly doesn't make execution much faster even for
    // problems with many boolean vars.
    return (values & other.values & masks[var]).any();
}

bool Values::abstracts(const Values &other) const {
    return other.values.is_subset_of(values);
}

void Values::get_possible_splits(const Values &flaw, const State conc_state,
                                 Splits *splits) const {
    assert(splits->empty());
    Bitset intersection(values & flaw.values);
    for (int var = 0; var < borders.size(); ++var) {
        if (!intersection.test(pos(var, conc_state[var]))) {
            vector<int> wanted;
            for (int pos = borders[var]; pos < borders[var] + g_variable_domain[var]; ++pos) {
                if (intersection.test(pos)) {
                    wanted.push_back(pos - borders[var]);
                }
            }
            splits->push_back(make_pair(var, wanted));
        }
    }
    assert(!splits->empty());
}

string Values::str() const {
    ostringstream oss;
    string sep = "";
    for (int var = 0; var < borders.size(); ++var) {
        int next_border = borders[var] + g_variable_domain[var];
        vector<int> facts;
        int pos = (var == 0) ? values.find_first() : values.find_next(borders[var] - 1);
        while (pos != Bitset::npos && pos < next_border) {
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
