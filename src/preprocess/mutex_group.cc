#include "mutex_group.h"

#include "helper_functions.h"
#include "variable.h"

MutexGroup::MutexGroup(istream &in, const vector<Variable *> &variables) {
    int size;
    check_magic(in, "begin_mutex_group");
    in >> size;
    for (size_t i = 0; i < size; ++i) {
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
    for (size_t i = 0; i < facts.size(); ++i) {
        const Variable *var = facts[i].first;
        int value = facts[i].second;
        cout << "   " << var->get_name() << " = " << value
             << " (" << var->get_fact_name(value) << ")" << endl;
    }
}

void MutexGroup::generate_cpp_input(ofstream &outfile) const {
    outfile << "begin_mutex_group" << endl
            << facts.size() << endl;
    for (size_t i = 0; i < facts.size(); ++i) {
        outfile << facts[i].first->get_level()
                << " " << facts[i].second << endl;
    }
    outfile << "end_mutex_group" << endl;
}

void MutexGroup::strip_unimportant_facts() {
    int new_index = 0;
    for (int i = 0; i < facts.size(); i++) {
        if (facts[i].first->get_level() != -1)
            facts[new_index++] = facts[i];
    }
    facts.erase(facts.begin() + new_index, facts.end());
}

bool MutexGroup::is_redundant() const {
    // Only mutex groups that talk about two or more different
    // finite-domain variables are interesting.
    for (int i = 1; i < facts.size(); ++i)
        if (facts[i].first != facts[i - 1].first)
            return false;
    return true;
}

void strip_mutexes(vector<MutexGroup> &mutexes) {
    int old_count = mutexes.size();
    int new_index = 0;
    for (int i = 0; i < mutexes.size(); i++) {
        mutexes[i].strip_unimportant_facts();
        if (!mutexes[i].is_redundant())
            mutexes[new_index++] = mutexes[i];
    }
    mutexes.erase(mutexes.begin() + new_index, mutexes.end());
    cout << mutexes.size() << " of " << old_count
         << " mutex groups necessary." << endl;
}
