#include "mutex_group.h"

#include "helper_functions.h"
#include "variable.h"

#include <fstream>
#include <iostream>

MutexGroup::MutexGroup(istream &in, const vector<Variable *> &variables) {
    int size;
    check_magic(in, "begin_mutex_group");
    in >> size;
    for (int i = 0; i < size; ++i) {
        int var_no, value;
        in >> var_no >> value;
        facts.push_back(make_pair(variables[var_no], value));
    }
    check_magic(in, "end_mutex_group");
}

int MutexGroup::get_encoding_size() const {
    return facts.size();
}

void MutexGroup::dump() const {
    cout << "mutex group of size " << facts.size() << ":" << endl;
    for (const auto &fact : facts) {
        const Variable *var = fact.first;
        int value = fact.second;
        cout << "   " << var->get_name() << " = " << value
             << " (" << var->get_fact_name(value) << ")" << endl;
    }
}

void MutexGroup::generate_cpp_input(ofstream &outfile) const {
    outfile << "begin_mutex_group" << endl
            << facts.size() << endl;
    for (const auto &fact : facts) {
        outfile << fact.first->get_level()
                << " " << fact.second << endl;
    }
    outfile << "end_mutex_group" << endl;
}

void MutexGroup::strip_unimportant_facts() {
    int new_index = 0;
    for (const auto &fact : facts) {
        if (fact.first->get_level() != -1)
            facts[new_index++] = fact;
    }
    facts.erase(facts.begin() + new_index, facts.end());
}

bool MutexGroup::is_redundant() const {
    // Only mutex groups that talk about two or more different
    // finite-domain variables are interesting.
    int num_facts = facts.size();
    for (int i = 1; i < num_facts; ++i)
        if (facts[i].first != facts[i - 1].first)
            return false;
    return true;
}

void strip_mutexes(vector<MutexGroup> &mutexes) {
    int old_count = mutexes.size();
    int new_index = 0;
    for (MutexGroup &mutex : mutexes) {
        mutex.strip_unimportant_facts();
        if (!mutex.is_redundant())
            mutexes[new_index++] = mutex;
    }
    mutexes.erase(mutexes.begin() + new_index, mutexes.end());
    cout << mutexes.size() << " of " << old_count
         << " mutex groups necessary." << endl;
}
