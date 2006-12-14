#ifndef STATE_H
#define STATE_H

#include <iostream>
#include <vector>
using namespace std;

class Operator;

class State {
    vector<int> vars; // values for vars
public:
    State(istream &in);
    State(const State &predecessor, const Operator &op);
    int &operator[](int index) {
	return vars[index];
    }
    int operator[](int index) const {
	return vars[index];
    }
    void dump() const;
    bool operator<(const State &other) const;
};

#endif
