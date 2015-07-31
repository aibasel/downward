#include "helper_functions.h"
#include "operator.h"
#include "variable.h"

#include <cassert>
#include <iostream>
#include <fstream>
using namespace std;

Operator::Operator(istream &in, const vector<Variable *> &variables) {
    check_magic(in, "begin_operator");
    in >> ws;
    getline(in, name);
    int count; // number of prevail conditions
    in >> count;
    for (int i = 0; i < count; i++) {
        int varNo, val;
        in >> varNo >> val;
        prevail.push_back(Prevail(variables[varNo], val));
    }
    in >> count; // number of pre_post conditions
    for (int i = 0; i < count; i++) {
        int eff_conds;
        vector<EffCond> ecs;
        in >> eff_conds;
        for (int j = 0; j < eff_conds; j++) {
            int var, value;
            in >> var >> value;
            ecs.push_back(EffCond(variables[var], value));
        }
        int varNo, val, newVal;
        in >> varNo >> val >> newVal;
        if (eff_conds)
            pre_post.push_back(PrePost(variables[varNo], ecs, val, newVal));
        else
            pre_post.push_back(PrePost(variables[varNo], val, newVal));
    }
    in >> cost;
    check_magic(in, "end_operator");
    // TODO: Evtl. effektiver: conditions schon sortiert einlesen?
}

void Operator::dump() const {
    cout << name << ":" << endl;
    cout << "prevail:";
    for (const auto &prev : prevail)
        cout << "  " << prev.var->get_name() << " := " << prev.prev;
    cout << endl;
    cout << "pre-post:";
    for (const auto &eff : pre_post) {
        if (eff.is_conditional_effect) {
            cout << "  if (";
            for (const auto &cond : eff.effect_conds)
                cout << cond.var->get_name() << " := " << cond.cond;
            cout << ") then";
        }
        cout << " " << eff.var->get_name() << " : "
             << eff.pre << " -> " << eff.post;
    }
    cout << endl;
}

int Operator::get_encoding_size() const {
    int size = 1 + prevail.size();
    for (const auto &eff : pre_post) {
        size += 1 + eff.effect_conds.size();
        if (eff.pre != -1)
            size += 1;
    }
    return size;
}

void Operator::strip_unimportant_effects() {
    int new_index = 0;
    for (const auto &eff : pre_post) {
        if (eff.var->get_level() != -1)
            pre_post[new_index++] = eff;
    }
    pre_post.erase(pre_post.begin() + new_index, pre_post.end());
}

bool Operator::is_redundant() const {
    return pre_post.empty();
}

void strip_operators(vector<Operator> &operators) {
    int old_count = operators.size();
    int new_index = 0;
    for (Operator &op : operators) {
        op.strip_unimportant_effects();
        if (!op.is_redundant())
            operators[new_index++] = op;
    }
    operators.erase(operators.begin() + new_index, operators.end());
    cout << operators.size() << " of " << old_count << " operators necessary." << endl;
}

void Operator::generate_cpp_input(ofstream &outfile) const {
    //TODO: beim Einlesen in search feststellen, ob leerer Operator
    outfile << "begin_operator" << endl;
    outfile << name << endl;

    outfile << prevail.size() << endl;
    for (const auto &prev : prevail) {
        assert(prev.var->get_level() != -1);
        if (prev.var->get_level() != -1)
            outfile << prev.var->get_level() << " " << prev.prev << endl;
    }

    outfile << pre_post.size() << endl;
    for (const auto &eff : pre_post) {
        assert(eff.var->get_level() != -1);
        outfile << eff.effect_conds.size();
        for (const auto &cond : eff.effect_conds)
            outfile << " " << cond.var->get_level()
                    << " " << cond.cond;
        outfile << " " << eff.var->get_level()
                << " " << eff.pre
                << " " << eff.post << endl;
    }
    outfile << cost << endl;
    outfile << "end_operator" << endl;
}
