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
    for (int i = 0; i < prevail.size(); i++)
        cout << "  " << prevail[i].var->get_name() << " := " << prevail[i].prev;
    cout << endl;
    cout << "pre-post:";
    for (int i = 0; i < pre_post.size(); i++) {
        if (pre_post[i].is_conditional_effect) {
            cout << "  if (";
            for (int j = 0; j < pre_post[i].effect_conds.size(); j++)
                cout << pre_post[i].effect_conds[j].var->get_name() << " := " <<
                pre_post[i].effect_conds[j].cond;
            cout << ") then";
        }
        cout << " " << pre_post[i].var->get_name() << " : " <<
        pre_post[i].pre << " -> " << pre_post[i].post;
    }
    cout << endl;
}

int Operator::get_encoding_size() const {
    int size = 1 + prevail.size();
    for (int i = 0; i < pre_post.size(); i++) {
        size += 1 + pre_post[i].effect_conds.size();
        if (pre_post[i].pre != -1)
            size += 1;
    }
    return size;
}

void Operator::strip_unimportant_effects() {
    int new_index = 0;
    for (int i = 0; i < pre_post.size(); i++) {
        if (pre_post[i].var->get_level() != -1)
            pre_post[new_index++] = pre_post[i];
    }
    pre_post.erase(pre_post.begin() + new_index, pre_post.end());
}

bool Operator::is_redundant() const {
    return pre_post.empty();
}

void strip_operators(vector<Operator> &operators) {
    int old_count = operators.size();
    int new_index = 0;
    for (int i = 0; i < operators.size(); i++) {
        operators[i].strip_unimportant_effects();
        if (!operators[i].is_redundant())
            operators[new_index++] = operators[i];
    }
    operators.erase(operators.begin() + new_index, operators.end());
    cout << operators.size() << " of " << old_count << " operators necessary." << endl;
}

void Operator::generate_cpp_input(ofstream &outfile) const {
    //TODO: beim Einlesen in search feststellen, ob leerer Operator
    outfile << "begin_operator" << endl;
    outfile << name << endl;

    outfile << prevail.size() << endl;
    for (int i = 0; i < prevail.size(); i++) {
        assert(prevail[i].var->get_level() != -1);
        if (prevail[i].var->get_level() != -1)
            outfile << prevail[i].var->get_level() << " " << prevail[i].prev << endl;
    }

    outfile << pre_post.size() << endl;
    for (int i = 0; i < pre_post.size(); i++) {
        assert(pre_post[i].var->get_level() != -1);
        outfile << pre_post[i].effect_conds.size();
        for (int j = 0; j < pre_post[i].effect_conds.size(); j++)
            outfile << " " << pre_post[i].effect_conds[j].var->get_level()
                    << " " << pre_post[i].effect_conds[j].cond;
        outfile << " " << pre_post[i].var->get_level()
                << " " << pre_post[i].pre
                << " " << pre_post[i].post << endl;
    }
    outfile << cost << endl;
    outfile << "end_operator" << endl;
}
