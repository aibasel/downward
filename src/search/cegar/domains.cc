#include "domains.h"

#include "../task_proxy.h"
#include "../utilities.h"

#include <sstream>

using namespace std;

namespace cegar {
vector<int> Domains::orig_domain_sizes;
int Domains::num_facts = -1;
vector<int> Domains::borders;
vector<Bitset> Domains::masks;
vector<Bitset> Domains::inverse_masks;
Bitset Domains::temp_bits;

Domains::Domains(TaskProxy task_proxy) {
    initialize_static_members(task_proxy);
    bits = Bitset(num_facts);
    bits.set();
}

void Domains::initialize_static_members(TaskProxy task_proxy) {
    orig_domain_sizes.clear();
    orig_domain_sizes.reserve(task_proxy.get_variables().size());
    for (VariableProxy var : task_proxy.get_variables())
        orig_domain_sizes.push_back(var.get_domain_size());

    masks.clear();
    inverse_masks.clear();
    borders.clear();
    num_facts = 0;
    int num_vars = orig_domain_sizes.size();
    for (int var = 0; var < num_vars; ++var) {
        borders.push_back(num_facts);
        num_facts += orig_domain_sizes[var];
    }
    for (int var = 0; var < num_vars; ++var) {
        // -----0000 -> --1110000 --> 001110000
        Bitset mask(borders[var]);
        mask.resize(borders[var] + orig_domain_sizes[var], true);
        mask.resize(num_facts, false);
        masks.push_back(mask);
        inverse_masks.push_back(~mask);
    }
    temp_bits.resize(num_facts);
}

void Domains::add(int var, int value) {
    bits.set(pos(var, value));
}

void Domains::remove(int var, int value) {
    bits.reset(pos(var, value));
}

void Domains::set(int var, int value) {
    remove_all(var);
    add(var, value);
}

void Domains::add_all(int var) {
    bits |= masks[var];
}

void Domains::remove_all(int var) {
    bits &= inverse_masks[var];
}

bool Domains::test(int var, int value) const {
    return bits.test(pos(var, value));
}

size_t Domains::count(int var) const {
    // Profiling showed that an explicit loop is faster than doing:
    // (values & masks[var]).count(); (even if using a temp bitset).
    int num_values = 0;
    for (int pos = borders[var]; pos < borders[var] + orig_domain_sizes[var]; ++pos) {
        num_values += bits.test(pos);
    }
    return num_values;
}

bool Domains::intersects(const Domains &other, int var) const {
    // Using test() directly doesn't make execution much faster even for
    // problems with many boolean vars. We use temp_values to reduce
    // memory allocations. This substantially reduces the relative time
    // spent in this method.
    temp_bits.set();
    temp_bits &= bits;
    temp_bits &= other.bits;
    temp_bits &= masks[var];
    assert(temp_bits.any() == (bits & other.bits & masks[var]).any());
    return temp_bits.any();
}

bool Domains::abstracts(const Domains &other) const {
    return other.bits.is_subset_of(bits);
}

void Domains::get_possible_flaws(const Domains &flaw,
                                 const State &conc_state,
                                 Flaws *flaws) const {
    assert(flaws->empty());
    Bitset intersection(bits & flaw.bits);
    for (size_t var = 0; var < borders.size(); ++var) {
        if (!intersection.test(pos(var, conc_state[var].get_value()))) {
            vector<int> wanted;
            for (int pos = borders[var]; pos < borders[var] + orig_domain_sizes[var]; ++pos) {
                if (intersection.test(pos)) {
                    wanted.push_back(pos - borders[var]);
                }
            }
            flaws->push_back(make_pair(var, wanted));
        }
    }
    assert(!flaws->empty());
}

ostream &operator<<(ostream &os, const Domains &domains) {
    string var_sep = "";
    os << "<";
    for (size_t var = 0; var < domains.borders.size(); ++var) {
        size_t next_border = domains.borders[var] + domains.orig_domain_sizes[var];
        vector<int> values;
        size_t pos = (var == 0) ?
                     domains.bits.find_first() :
                     domains.bits.find_next(domains.borders[var] - 1);
        while (pos != Bitset::npos && pos < next_border) {
            values.push_back(pos - domains.borders[var]);
            pos = domains.bits.find_next(pos);
        }
        assert(!values.empty());
        if (static_cast<int>(values.size()) < domains.orig_domain_sizes[var]) {
            os << var_sep << var << "={";
            string value_sep = "";
            for (int value : values) {
                os << value_sep << value;
                value_sep = ",";
            }
            os << "}";
            var_sep = ",";
        }
    }
    return os << ">";
}
}
