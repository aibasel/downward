#ifndef VARIABLE_H
#define VARIABLE_H

#include <iostream>
using namespace std;

class Variable {
    int range;
    string name;
    int layer;
    int level;
    bool necessary;
public:
    Variable(istream &in);
    void set_level(int level);
    void set_necessary();
    int get_level() const;
    bool is_necessary() const;
    int get_range() const;
    string get_name() const;
    int get_layer() const {return layer; }
    bool is_derived() const {return layer != -1; }
    void dump() const;
};

#endif
