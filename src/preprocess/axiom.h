#ifndef AXIOM_H
#define AXIOM_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

class Variable;

class Axiom {
public:
    struct Condition {
        Variable *var;
        int cond;
        Condition(Variable *v, int c) : var(v), cond(c) {}
    };
private:
    Variable *effect_var;
    int old_val;
    int effect_val;
    vector<Condition> conditions;    // var, val
public:
    Axiom(istream &in, const vector<Variable *> &variables);

    bool is_redundant() const;
    void dump() const;
    int get_encoding_size() const;
    void generate_cpp_input(ofstream &outfile) const;
    const vector<Condition> &get_conditions() const {return conditions; }
    Variable *get_effect_var() const {return effect_var; }
    int get_old_val() const {return old_val; }
    int get_effect_val() const {return effect_val; }
};

extern void strip_axioms(vector<Axiom> &axioms);

#endif
