#include "domains.h"

#include <sstream>

using namespace std;

namespace cegar {
vector<int> Domains::orig_domain_sizes;
int Domains::num_facts = -1;
vector<int> Domains::borders;
vector<Bitset> Domains::masks;
vector<Bitset> Domains::inverse_masks;
Bitset Domains::temp_bits;

Domains::Domains(vector<int> &&domain_sizes) {
    initialize_static_members(move(domain_sizes));
    bits = Bitset(num_facts);
    bits.set();
}

void Domains::initialize_static_members(vector<int> &&domain_sizes) {
    orig_domain_sizes = domain_sizes;

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

size_t Domains::count(int var) const {
    /* Profiling showed that an explicit loop is faster than intersecting with
       a mask and calling dynamic_bitset::count(). */
    int num_values = 0;
    for (int value = 0; value < orig_domain_sizes[var]; ++value) {
        num_values += test(var, value);
    }
    return num_values;
}

bool Domains::intersects(const Domains &other, int var) const {
    /*
      Using test() directly is usually a bit slower, even for problems with
      many boolean vars. We use a temporary bitset to reduce memory
      allocations. This substantially reduces the time spent in this method.
    */
    temp_bits.set();
    temp_bits &= bits;
    temp_bits &= other.bits;
    temp_bits &= masks[var];
    assert(temp_bits.any() == (bits & other.bits & masks[var]).any());
    return temp_bits.any();
}

bool Domains::is_superset_of(const Domains &other) const {
    return other.bits.is_subset_of(bits);
}

ostream &operator<<(ostream &os, const Domains &domains) {
    string var_sep = "";
    os << "<";
    for (size_t var = 0; var < domains.orig_domain_sizes.size(); ++var) {
        vector<int> values;
        for (int value = 0; value < domains.orig_domain_sizes[var]; ++value) {
            if (domains.test(var, value))
                values.push_back(value);
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
