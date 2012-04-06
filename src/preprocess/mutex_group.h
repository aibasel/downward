#ifndef MUTEX_GROUP_H
#define MUTEX_GROUP_H

#include <iostream>
#include <vector>
using namespace std;

class Variable;

class MutexGroup {
    vector<pair<const Variable *, int> > facts;
public:
    MutexGroup(istream &in, const vector<Variable *> &variables);

    void strip_unimportant_facts();
    bool is_redundant() const;

    int get_encoding_size() const;
    void generate_cpp_input(ofstream &outfile) const;
    void dump() const;
};

extern void strip_mutexes(vector<MutexGroup> &mutexes);

#endif
