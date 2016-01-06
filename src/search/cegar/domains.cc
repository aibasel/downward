#include "domains.h"

#include <sstream>

using namespace std;

namespace CEGAR {
vector<int> Domains::orig_domain_sizes;
int Domains::num_facts = -1;
vector<int> Domains::borders;
vector<Bitset> Domains::masks;
vector<Bitset> Domains::inverse_masks;
Bitset Domains::temp_bits;

Domains::Domains(vector<int> &&domain_sizes) {
    initialize_static_members(move(domain_sizes));
    for (int domain_size : orig_domain_sizes) {
        Bitset domain(domain_size);
        domain.set();
        bits.push_back(move(domain));
    }
}

void Domains::initialize_static_members(vector<int> &&domain_sizes) {
    orig_domain_sizes = move(domain_sizes);

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
        Bitset mask(borders[var]);
        // Example mask: 0000-----
        mask.resize(borders[var] + orig_domain_sizes[var], true);
        // Example mask: 0000111--
        mask.resize(num_facts, false);
        // Example mask: 000011100
        masks.push_back(mask);
        inverse_masks.push_back(~mask);
    }
    temp_bits.resize(num_facts);
}

void Domains::add(int var, int value) {
    bits[var].set(value);
}

void Domains::remove(int var, int value) {
    bits[var].reset(value);
}

void Domains::set_single_value(int var, int value) {
    remove_all(var);
    add(var, value);
}

void Domains::add_all(int var) {
    bits[var].set();
}

void Domains::remove_all(int var) {
    bits[var].reset();
}

int Domains::count(int var) const {
    return bits[var].count();
}

bool Domains::intersects(const Domains &other, int var) const {
    return bits[var].intersects(other.bits[var]);
}

bool Domains::is_superset_of(const Domains &other) const {
    int num_vars = bits.size();
    for (int var = 0; var < num_vars; ++var) {
        if (!other.bits[var].is_subset_of(bits[var]))
            return false;
    }
    return true;
}

ostream &operator<<(ostream &os, const Domains &domains) {
    int num_vars = domains.bits.size();
    string var_sep = "";
    os << "<";
    for (int var = 0; var < num_vars; ++var) {
        const Bitset &domain = domains.bits[var];
        vector<int> values;
        for (size_t value = 0; value < domain.size(); ++value) {
            if (domain[value])
                values.push_back(value);
        }
        assert(!values.empty());
        if (values.size() < domain.size()) {
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
