#ifndef CEGAR_UTILS_H
#define CEGAR_UTILS_H

#include <ext/slist>
#include <vector>
#include <set>
#include <iostream>
#include <sstream>

#include "../heuristic.h"
#include "../operator.h"
#include "./abstract_state.h"
#include "./abstraction.h"

#include "gtest/gtest_prod.h"

namespace cegar_heuristic {

// Gtest prevents us from defining this variable in the header.
extern int UNDEFINED;

std::string int_set_to_string(std::set<int> myset);

int get_eff(Operator op, int var);
int get_pre(Operator op, int var);

void get_unmet_preconditions(const Operator &op, const State &s,
                             std::vector<pair<int,int> > *cond);

// Create an operator with cost 1.
// prevails have the form "var value".
// pre_posts have the form "0 var pre post" (no conditional effects).
Operator create_op(const std::string desc);
Operator create_op(const std::string name, std::vector<string> prevail,
                   std::vector<string> pre_post);

State* create_state(const std::string desc);

}

#endif
