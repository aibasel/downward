#include "domains.h"

#include <sstream>

using namespace std;

namespace CEGAR {
Domains::Domains(const vector<int> &domain_sizes) {
    bits.reserve(domain_sizes.size());
    for (int domain_size : domain_sizes) {
        Bitset domain(domain_size);
        domain.set();
        bits.push_back(move(domain));
    }
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
