#ifndef OPERATOR_H
#define OPERATOR_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

class Variable;

class Operator {
public:
    struct Prevail {
        Variable *var;
        int prev;
        Prevail(Variable *v, int p) : var(v), prev(p) {}
    };
    struct EffCond {
        Variable *var;
        int cond;
        EffCond(Variable *v, int c) : var(v), cond(c) {}
    };
    struct PrePost {
        Variable *var;
        int pre, post;
        bool is_conditional_effect;
        vector<EffCond> effect_conds;
        PrePost(Variable *v, int pr, int po) : var(v), pre(pr), post(po) {
            is_conditional_effect = false;
        }
        PrePost(Variable *v, vector<EffCond> ecs, int pr, int po) : var(v), pre(pr),
                                                                    post(po), effect_conds(ecs) {is_conditional_effect = true; }
    };

private:
    string name;
    vector<Prevail> prevail;    // var, val
    vector<PrePost> pre_post; // var, old-val, new-val
    int cost;
public:
    Operator(istream &in, const vector<Variable *> &variables);

    void strip_unimportant_effects();
    bool is_redundant() const;

    void dump() const;
    int get_encoding_size() const;
    void generate_cpp_input(ofstream &outfile) const;
    int get_cost() const {return cost; }
    string get_name() const {return name; }
    const vector<Prevail> &get_prevail() const {return prevail; }
    const vector<PrePost> &get_pre_post() const {return pre_post; }
};

extern void strip_operators(vector<Operator> &operators);

#endif
