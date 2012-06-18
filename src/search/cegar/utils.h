#ifndef CEGAR_UTILS_H
#define CEGAR_UTILS_H

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../operator.h"
#include "../state.h"

namespace cegar_heuristic {

const bool DEBUG = false;

const int UNDEFINED = -1;

std::string int_set_to_string(std::set<int> myset);

void get_prevail_and_preconditions(const Operator &op, vector<pair<int, int> > *cond);

int get_eff(const Operator &op, int var);
int get_pre(const Operator &op, int var);

void get_unmet_preconditions(const Operator &op, const State &s,
                             std::vector<pair<int, int> > *cond);

void get_unmet_goal_conditions(const State &state,
                               vector<pair<int, int> > *unmet_conditions);

bool goal_var(int var);

// Create an operator with cost 1.
// prevails have the form "var value".
// pre_posts have the form "0 var pre post" (no conditional effects).
Operator create_op(const std::string desc);
Operator create_op(const std::string name, std::vector<string> prevail,
                   std::vector<string> pre_post);

State *create_state(const std::string desc);
}

#endif
