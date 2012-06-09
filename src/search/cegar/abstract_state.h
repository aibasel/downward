#ifndef CEGAR_ABSTRACT_STATE_H
#define CEGAR_ABSTRACT_STATE_H

#include <ext/slist>
#include <vector>
#include <set>
#include <iostream>
#include <sstream>

#include "../heuristic.h"
#include "../operator.h"

#include "gtest/gtest_prod.h"

namespace cegar_heuristic {

class AbstractState;
typedef pair<Operator, AbstractState> Arc;

// TODO(jendrik): Use 32-bit masks for variables. This means we can not handle tasks
// with domain sizes > 32.

int get_eff(Operator op, int var);
int get_pre(Operator op, int var);

// Create an operator with cost 1.
// prevails have the form "var value".
// pre_posts have the form "0 var pre post" (no conditional effects).
Operator create_op(const std::string desc);
Operator create_op(const std::string name, std::vector<string> prevail,
                   std::vector<string> pre_post);

class AbstractState {
private:
    FRIEND_TEST(CegarTest, regress);
    // Possible values of each variable in this state.
    // values[1] == {2} -> var1 is concretely set here.
    // values[1] == {2, 3} -> var1 has two possible values.
    // values[1] == {} -> var1 has all possible values.
    std::vector<std::set<int> > values;

    std::vector<Arc> next, prev;

public:
    AbstractState(string s="");
    void regress(const Operator &op, AbstractState *result) const;
    string str() const;
    set<int> get_values(int var) const;
    void set_value(int var, int value);
    //void remove_value(int var, value);
    void refine(int var, int value, AbstractState *v1, AbstractState *v2);
    bool operator==(AbstractState &other) const;
    bool operator!=(AbstractState &other) const;
    void add_arc(Operator &op, AbstractState &other);
    void remove_arc(Operator &op, AbstractState &other);
    bool check_arc(Operator &op, AbstractState &other);
    bool applicable(const Operator &op) const;
    void apply(const Operator &op, AbstractState *result) const;
    bool agrees_with(const AbstractState &other) const;
};

}

#endif
