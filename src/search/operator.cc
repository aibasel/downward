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

        if (g_legacy_file_format) {
            cost = 1;
        } else {
            int op_cost;
            in >> op_cost;
            cost = g_use_metric ? op_cost : 1;
        }
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

void Prevail::dump() const {
    cout << g_variable_name[var] << ": " << prev;
}

void PrePost::dump() const {
    cout << g_variable_name[var] << ": " << pre << " => " << post;
    if (!cond.empty()) {
        cout << " if";
        for (int i = 0; i < cond.size(); i++) {
            cout << " ";
            cond[i].dump();
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


/* [raz-ipc-integration] HACK HACK HACK: The following
   code was added for the IPC integration.
 */

//TODO - not using this now - verify that this method actually gives the distance...
int Operator::get_distance_from_op(const Operator *other_op) {
    int distance = 0;
    //PRE_POST
    const vector<PrePost> &other_pre_post = other_op->pre_post;
    for (int cond_idx = 0; cond_idx < pre_post.size(); cond_idx++) {
        bool contains_condition = false;
        for (int other_cond_idx = 0; other_cond_idx < other_pre_post.size(); other_cond_idx++) {
            if (pre_post[cond_idx] == other_pre_post[other_cond_idx]) {
                contains_condition = true;
                break;
            }
        }
        if (!contains_condition)
            distance++;
    }

    for (int other_cond_idx = 0; other_cond_idx < other_pre_post.size(); other_cond_idx++) {
        bool contains_condition = false;
        for (int cond_idx = 0; cond_idx < pre_post.size(); cond_idx++) {
            if (pre_post[cond_idx] == other_pre_post[other_cond_idx]) {
                contains_condition = true;
                break;
            }
        }
        if (!contains_condition)
            distance++;
    }
    //PREVAIL
    const vector<Prevail> &other_prevail = other_op->prevail;
    for (int cond_idx = 0; cond_idx < prevail.size(); cond_idx++) {
        bool contains_condition = false;
        for (int other_cond_idx = 0; other_cond_idx < other_prevail.size(); other_cond_idx++) {
            if (prevail[cond_idx] == other_prevail[other_cond_idx]) {
                contains_condition = true;
                break;
            }
        }
        if (!contains_condition)
            distance++;
    }

    for (int other_cond_idx = 0; other_cond_idx < other_prevail.size(); other_cond_idx++) {
        bool contains_condition = false;
        for (int cond_idx = 0; cond_idx < prevail.size(); cond_idx++) {
            if (prevail[cond_idx] == other_prevail[other_cond_idx]) {
                contains_condition = true;
                break;
            }
        }
        if (!contains_condition)
            distance++;
    }
    return distance;
}

bool Operator::have_same_op_name(const Operator *other_op) const {
    return name.compare(0, name.find(' '), other_op->name.substr(0, other_op->name.find(' '))) == 0;
}

bool Operator::affect_a_common_variable(const Operator *other_op) const {
    for (int i = 0; i < pre_post.size(); i++) {
        int var = pre_post[i].var;
        for (int j = 0; j < other_op->pre_post.size(); j++)
            if (other_op->pre_post[j].var == var)
                return true;
    }
    return false;
}
