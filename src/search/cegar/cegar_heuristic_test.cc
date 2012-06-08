#include <gtest/gtest.h>

#include "./cegar_heuristic.h"
#include "./../operator.h"

#include <iostream>

using namespace std;



namespace cegar_heuristic {

//vector<int> g_variable_domain;

void init_test() {
    // We have to construct the variable domains.
    // 0 in {0, 1}, 1 in {0, 1, 2}.
    g_variable_domain.clear();
    g_variable_domain.push_back(2);
    g_variable_domain.push_back(3);
    //g_variable_domain.push_back(2);
}

TEST(CegarTest, str) {
    init_test();

    AbstractState a;
    vector<string> states;
    states.push_back("<0={1}>");
    states.push_back("<>");
    states.push_back("<0={0,1},1={0,2}>");
    states.push_back("<0={0},1={0,1}>");
    for (int i = 0; i < states.size(); ++i) {
        a = AbstractState(states[i]);
        ASSERT_EQ(states[i], a.str());
    }
}


TEST(CegarTest, regress) {
    init_test();

    // Operator: <0=0, 1=0 --> 1=1>
    vector<string> prevail;
    prevail.push_back("0 0");
    vector<string> pre_post;
    pre_post.push_back("0 1 0 1");
    Operator op = create_op("op", prevail, pre_post);

    AbstractState a;
    AbstractState b;

    vector<pair<string, string> > pairs;

    pairs.push_back(pair<string, string>("<0={0},1={0}>", "<0={0},1={0,1}>"));
    pairs.push_back(pair<string, string>("<0={0},1={0}>", "<>"));

    for (int i = 0; i < pairs.size(); ++i) {
        b = AbstractState(pairs[i].second);
        a = b.regress(op);
        ASSERT_EQ(pairs[i].first, a.str());
    }
}

}
