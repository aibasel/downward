#include "values.h"

#include "../task_proxy.h"

#include <sstream>

using namespace std;

namespace cegar {
vector<int> Values::variable_domain;
int Values::facts = -1;
vector<int> Values::borders;
vector<Bitset> Values::masks;
vector<Bitset> Values::inverse_masks;
Bitset Values::temp_values;

Values::Values() {
    assert(facts >= 0 && "Static members have not been initialized.");
    values = Bitset(facts);
    values.set();
}

void Values::initialize_static_members(TaskProxy task) {
    variable_domain.clear();
    variable_domain.reserve(task.get_variables().size());
    for (VariableProxy var : task.get_variables())
        variable_domain.push_back(var.get_domain_size());

    masks.clear();
    inverse_masks.clear();
    borders.clear();
    facts = 0;
    int num_vars = variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        borders.push_back(facts);
        facts += variable_domain[var];
    }
    for (int var = 0; var < num_vars; ++var) {
        // -----0000 -> --1110000 --> 001110000
        Bitset mask(borders[var]);
        mask.resize(borders[var] + variable_domain[var], true);
        mask.resize(facts, false);
        masks.push_back(mask);
        inverse_masks.push_back(~mask);
    }
    temp_values.resize(facts);
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
    values &= inverse_masks[var];
}

bool Values::test(int var, int value) const {
    return values.test(pos(var, value));
}

size_t Values::count(int var) const {
    // Profiling showed that an explicit loop is faster than doing:
    // (values & masks[var]).count(); (even if using a temp bitset).
    int num_values = 0;
    for (int pos = borders[var]; pos < borders[var] + variable_domain[var]; ++pos) {
        num_values += values.test(pos);
    }
    return num_values;
}

bool Values::domains_intersect(const Values &other, int var) {
    // Using test() directly doesn't make execution much faster even for
    // problems with many boolean vars. We use temp_values to reduce
    // memory allocations. This substantially reduces the relative time
    // spent in this method.
    temp_values.set();
    temp_values &= values;
    temp_values &= other.values;
    temp_values &= masks[var];
    assert(temp_values.any() == (values & other.values & masks[var]).any());
    return temp_values.any();
}

bool Values::abstracts(const Values &other) const {
    return other.values.is_subset_of(values);
}

void Values::get_possible_splits(const Values &flaw, const State &conc_state,
                                 Splits *splits) const {
    assert(splits->empty());
    Bitset intersection(values & flaw.values);
    for (size_t var = 0; var < borders.size(); ++var) {
        if (!intersection.test(pos(var, conc_state[var].get_value()))) {
            vector<int> wanted;
            for (int pos = borders[var]; pos < borders[var] + variable_domain[var]; ++pos) {
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
    for (size_t var = 0; var < borders.size(); ++var) {
        size_t next_border = borders[var] + variable_domain[var];
        vector<int> facts;
        size_t pos = (var == 0) ? values.find_first() : values.find_next(borders[var] - 1);
        while (pos != Bitset::npos && pos < next_border) {
            facts.push_back(pos - borders[var]);
            pos = values.find_next(pos);
        }
        assert(!facts.empty());
        if (facts.size() < static_cast<size_t>(variable_domain[var])) {
            oss << sep << var << "={";
            sep = ",";
            string value_sep = "";
            for (size_t i = 0; i < facts.size(); ++i) {
                oss << value_sep << facts[i];
                value_sep = ",";
            }
            oss << "}";
        }
    }
    return oss.str();
}
}
