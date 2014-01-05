#include "globals.h"
#include "operator.h"

#include <iostream>
using namespace std;

Prevail::Prevail(istream &in) {
    in >> var >> prev;
}

PrePost::PrePost(istream &in) {
    int condCount;
    in >> condCount;
    for (int i = 0; i < condCount; i++)
        cond.push_back(Prevail(in));
    in >> var >> pre >> post;
}

Operator::Operator(istream &in, bool axiom) {
    marked = false;

    is_an_axiom = axiom;
    if (!is_an_axiom) {
        check_magic(in, "begin_operator");
        in >> ws;
        getline(in, name);
        int count;
        in >> count;
        for (int i = 0; i < count; i++)
            prevail.push_back(Prevail(in));
        in >> count;
        for (int i = 0; i < count; i++)
            pre_post.push_back(PrePost(in));

        int op_cost;
        in >> op_cost;
        cost = g_use_metric ? op_cost : 1;

        g_min_action_cost = min(g_min_action_cost, cost);
        g_max_action_cost = max(g_max_action_cost, cost);

        check_magic(in, "end_operator");
    } else {
        name = "<axiom>";
        cost = 0;
        check_magic(in, "begin_rule");
        pre_post.push_back(PrePost(in));
        check_magic(in, "end_rule");
    }

    marker1 = marker2 = false;
}

void rename_fact_in_conditions(int variable, int before, int after, vector<Prevail> &conditions) {
    // TODO: Break out of loop when var > variable once conditions are sorted.
    for (int j = 0; j < conditions.size(); ++j) {
        Prevail &cond = conditions[j];
        if (cond.var == variable && cond.prev == before) {
            cond.prev = after;
            // Each variable appears in at most one effect condition.
            break;
        }
    }
}

void Operator::rename_fact(int variable, int before, int after) {
    // TODO: Break out of loop when var > variable once conditions are sorted.
    rename_fact_in_conditions(variable, before, after, prevail);
    for (int i = 0; i < pre_post.size(); ++i) {
        PrePost &prepost = pre_post[i];
        if (prepost.var == variable) {
            if (prepost.pre == before) {
                prepost.pre = after;
            } else if (prepost.post == before) {
                prepost.post = after;
            }
        }
        rename_fact_in_conditions(variable, before, after, prepost.cond);
    }
}

void Prevail::dump() const {
    cout << g_variable_name[var] << ": " << var << "=" << prev;
}

void PrePost::dump() const {
    cout << g_variable_name[var] << ": " << var << "=" << pre << " => " << post;
    if (!cond.empty()) {
        cout << " if";
        for (int i = 0; i < cond.size(); i++) {
            cout << " ";
            cond[i].dump();
        }
    }
}

void Operator::keep_single_effect(int var, int value) {
    for(vector<PrePost>::iterator it = pre_post.begin(); it != pre_post.end();) {
        if (it->var == var and it->post == value) {
            ++it;
        } else {
            if (it->pre != -1)
                prevail.push_back(Prevail(it->var, it->pre));
            it = pre_post.erase(it);
        }
    }
}

void Operator::dump() const {
    cout << name << ":";
    for (int i = 0; i < prevail.size(); i++) {
        cout << " [";
        prevail[i].dump();
        cout << "]";
    }
    for (int i = 0; i < pre_post.size(); i++) {
        cout << " [";
        pre_post[i].dump();
        cout << "]";
    }
    cout << endl;
}
