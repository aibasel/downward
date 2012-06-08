#include <gtest/gtest.h>

#include "./cegar_heuristic.h"
#include "./../operator.h"

#include <iostream>

using namespace std;



namespace cegar_heuristic {

//vector<int> g_variable_domain;

TEST(CegarTest, str) {
    // We have to construct the variable domains.
    // 0 in {0, 1}, 1 in {0, 1, 2}.
    g_variable_domain.push_back(2);
    g_variable_domain.push_back(3);

    AbstractState a;
    string s = "<0={1}>";
    a = AbstractState(s);
    ASSERT_EQ(s, a.str());
}


TEST(CegarTest, regress) {
    // We have to construct the variable domains.
    // 0 in {0, 1}, 1 in {0, 1, 2}.
    g_variable_domain.push_back(2);
    g_variable_domain.push_back(3);

    AbstractState b = AbstractState();
    set<int> vals = b.values[0];
    b.values[0].insert(0);
    b.values[1].insert(0);
    b.values[1].insert(1);
    ASSERT_EQ("<0={0},1={0,1}>", b.str());

    // Operator: <0=0, 1=0 --> 1=1>
    vector<string> prevail;
    prevail.push_back("0 0");
    vector<string> pre_post;
    pre_post.push_back("0 1 0 1");
    Operator op = create_op("op", prevail, pre_post);

    AbstractState a = b.regress(op);
    cout << a.str();
    ASSERT_EQ("<0={0},1={0}>", a.str());

    AbstractState c = AbstractState("<0={1}>");
    ASSERT_EQ(16, 4*4);
}

}
