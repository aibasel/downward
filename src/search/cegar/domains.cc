#include "domains.h"

#include "../task_proxy.h"

#include <sstream>

using namespace std;

namespace cegar {
vector<int> Domains::original_domain_sizes;
int Domains::num_facts = -1;
vector<int> Domains::borders;
vector<Bitset> Domains::masks;
vector<Bitset> Domains::inverse_masks;
Bitset Domains::temp_values;

Domains::Domains(TaskProxy task_proxy) {
    initialize_static_members(task_proxy);
    domains = Bitset(num_facts);
    domains.set();
}

void Domains::initialize_static_members(TaskProxy task_proxy) {
    original_domain_sizes.clear();
    original_domain_sizes.reserve(task_proxy.get_variables().size());
    for (VariableProxy var : task_proxy.get_variables())
        original_domain_sizes.push_back(var.get_domain_size());

    masks.clear();
    inverse_masks.clear();
    borders.clear();
    num_facts = 0;
    int num_vars = original_domain_sizes.size();
    for (int var = 0; var < num_vars; ++var) {
        borders.push_back(num_facts);
        num_facts += original_domain_sizes[var];
    }
    for (int var = 0; var < num_vars; ++var) {
        // -----0000 -> --1110000 --> 001110000
        Bitset mask(borders[var]);
        mask.resize(borders[var] + original_domain_sizes[var], true);
        mask.resize(num_facts, false);
        masks.push_back(mask);
        inverse_masks.push_back(~mask);
    }
    temp_values.resize(num_facts);
}

void Domains::add(int var, int value) {
    domains.set(pos(var, value));
}

void Domains::remove(int var, int value) {
    domains.reset(pos(var, value));
}

void Domains::set(int var, int value) {
    remove_all(var);
    add(var, value);
}

void Domains::add_all(int var) {
    domains |= masks[var];
}

void Domains::remove_all(int var) {
    domains &= inverse_masks[var];
}

bool Domains::test(int var, int value) const {
    return domains.test(pos(var, value));
}

size_t Domains::count(int var) const {
    // Profiling showed that an explicit loop is faster than doing:
    // (values & masks[var]).count(); (even if using a temp bitset).
    int num_values = 0;
    for (int pos = borders[var]; pos < borders[var] + original_domain_sizes[var]; ++pos) {
        num_values += domains.test(pos);
    }
    return num_values;
}

bool Domains::intersects(const Domains &other, int var) const {
    // Using test() directly doesn't make execution much faster even for
    // problems with many boolean vars. We use temp_values to reduce
    // memory allocations. This substantially reduces the relative time
    // spent in this method.
    temp_values.set();
    temp_values &= domains;
    temp_values &= other.domains;
    temp_values &= masks[var];
    assert(temp_values.any() == (domains & other.domains & masks[var]).any());
    return temp_values.any();
}

bool Domains::abstracts(const Domains &other) const {
    return other.domains.is_subset_of(domains);
}

void Domains::get_possible_flaws(const Domains &flaw,
                                 const State &conc_state,
                                 Flaws *flaws) const {
    assert(flaws->empty());
    Bitset intersection(domains & flaw.domains);
    for (size_t var = 0; var < borders.size(); ++var) {
        if (!intersection.test(pos(var, conc_state[var].get_value()))) {
            vector<int> wanted;
            for (int pos = borders[var]; pos < borders[var] + original_domain_sizes[var]; ++pos) {
                if (intersection.test(pos)) {
                    wanted.push_back(pos - borders[var]);
                }
            }
            flaws->push_back(make_pair(var, wanted));
        }
    }
    assert(!flaws->empty());
}

string Domains::str() const {
    ostringstream oss;
    string sep = "";
    for (size_t var = 0; var < borders.size(); ++var) {
        size_t next_border = borders[var] + original_domain_sizes[var];
        vector<int> facts;
        size_t pos = (var == 0) ? domains.find_first() : domains.find_next(borders[var] - 1);
        while (pos != Bitset::npos && pos < next_border) {
            facts.push_back(pos - borders[var]);
            pos = domains.find_next(pos);
        }
        assert(!facts.empty());
        if (static_cast<int>(facts.size()) < original_domain_sizes[var]) {
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
