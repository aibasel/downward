#include "utils.h"

#include <cassert>
#include <set>
#include <sstream>
#include <vector>

#include "../globals.h"
#include "../operator.h"
#include "../state.h"

using namespace std;

namespace cegar_heuristic {

string int_set_to_string(set<int> myset) {
    ostringstream oss;
    oss << "{";
    int j = 0;
    for (set<int>::iterator iter = myset.begin(); iter != myset.end(); ++iter) {
        oss << *iter;
        ++j;
        if (j < myset.size())
            oss << ",";
    }
    oss << "}";
    return oss.str();
}

Operator create_op(const string name, vector<string> prevail, vector<string> pre_post) {
    ostringstream oss;
    // Create operator description.
    oss << name << endl << prevail.size() << endl;
    for (int i = 0; i < prevail.size(); ++i)
        oss << prevail[i] << endl;
    oss << pre_post.size() << endl;
    for (int i = 0; i < pre_post.size(); ++i)
        oss << pre_post[i] << endl;
    oss << 1;
    return create_op(oss.str());
}

Operator create_op(const std::string desc) {
    std::string full_op_desc = "begin_operator\n" + desc + "\nend_operator";
    istringstream iss(full_op_desc);
    Operator op = Operator(iss, false);
    return op;
}

State *create_state(const std::string desc) {
    std::string full_desc = "begin_state\n" + desc + "\nend_state";
    istringstream iss(full_desc);
    return new State(iss);
}

int get_eff(const Operator &op, int var) {
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        PrePost pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            return pre_post.post;
    }
    return UNDEFINED;
}

void get_prevail_and_preconditions(const Operator &op, vector<pair<int, int> > *cond) {
    for (int i = 0; i < op.get_prevail().size(); i++) {
        const Prevail *prevail = &op.get_prevail()[i];
        cond->push_back(pair<int, int>(prevail->var, prevail->prev));
    }
    for (int i = 0; i < op.get_pre_post().size(); i++) {
        const PrePost *pre_post = &op.get_pre_post()[i];
        cond->push_back(pair<int, int>(pre_post->var, pre_post->pre));
    }
}

int get_pre(const Operator &op, int var) {
    vector<pair<int, int> > preconditions;
    get_prevail_and_preconditions(op, &preconditions);
    for (int i = 0; i < preconditions.size(); ++i) {
        if (preconditions[i].first == var)
            return preconditions[i].second;
    }
    return UNDEFINED;
}

void get_unmet_preconditions(const Operator &op, const State &state,
                             vector<pair<int, int> > *cond) {
    assert(cond->empty());
    vector<pair<int, int> > preconditions;
    get_prevail_and_preconditions(op, &preconditions);
    for (int i = 0; i < preconditions.size(); i++) {
        int var = preconditions[i].first;
        int value = preconditions[i].second;
        if (value != -1 && state[var] != value)
            cond->push_back(pair<int, int>(var, value));
    }
    assert(cond->empty() == op.is_applicable(state));
}

void get_unmet_goal_conditions(const State &state,
                               vector<pair<int, int> > *unmet_conditions) {
    for (int i = 0; i < g_goal.size(); i++) {
        int var = g_goal[i].first;
        int value = g_goal[i].second;
        if (state[var] != value) {
            unmet_conditions->push_back(pair<int, int>(var, value));
        }
    }
}
}
