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

TEST(CegarTest, equal) {
    init_test();

    AbstractState a = AbstractState("<0={0},1={0}>");
    AbstractState b = AbstractState("<1={0},0={0}>");
    AbstractState c = AbstractState("<1={0}>");
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a == c);
    ASSERT_FALSE(b == c);
}

TEST(CegarTest, refine) {
    init_test();

    AbstractState a = AbstractState();
    a.refine(0, 1);
}

TEST(CegarTest, applicable) {
    init_test();

    // Operator: <0=0, 1=0 --> 1=1>
    vector<string> prevail;
    prevail.push_back("0 0");
    vector<string> pre_post;
    pre_post.push_back("0 1 0 1");
    Operator op = create_op("op", prevail, pre_post);

    vector<pair<string, bool> > pairs;

    pairs.push_back(pair<string, bool>("<>", true));
    pairs.push_back(pair<string, bool>("<0={0}>", true));
    pairs.push_back(pair<string, bool>("<0={0},1={0}>", true));
    pairs.push_back(pair<string, bool>("<0={1},1={0}>", false));
    pairs.push_back(pair<string, bool>("<0={1}>", false));
    pairs.push_back(pair<string, bool>("<1={1}>", false));

    AbstractState a;
    for (int i = 0; i < pairs.size(); ++i) {
        a = AbstractState(pairs[i].first);
        ASSERT_EQ(pairs[i].second, a.applicable(op));
    }
}

TEST(CegarTest, apply) {
    init_test();
    // Check that this variable doesn't appear in the resulting state.
    g_variable_domain.push_back(2);

    // Operator: <0=0, 1=0 --> 1=1>
    vector<string> prevail;
    prevail.push_back("0 0");
    vector<string> pre_post;
    pre_post.push_back("0 1 0 1");
    Operator op = create_op("op", prevail, pre_post);

    vector<pair<string, string> > pairs;

    pairs.push_back(pair<string, string>("<>", "<0={0},1={1}>"));
    pairs.push_back(pair<string, string>("<0={0}>", "<0={0},1={1}>"));
    pairs.push_back(pair<string, string>("<1={0}>", "<0={0},1={1}>"));
    pairs.push_back(pair<string, string>("<0={0},1={0}>", "<0={0},1={1}>"));

    AbstractState orig;
    AbstractState result;
    for (int i = 0; i < pairs.size(); ++i) {
        orig = AbstractState(pairs[i].first);
        orig.apply(op, &result);
        ASSERT_TRUE(AbstractState(pairs[i].second) == result);
    }
}

TEST(CegarTest, agrees_with) {
    init_test();
    // Check that this variable doesn't appear in the resulting state.
    g_variable_domain.push_back(2);

    vector<pair<string, string> > pairs;

    pairs.push_back(pair<string, string>("<>", "<0={0},1={1}>"));
    pairs.push_back(pair<string, string>("<0={0}>", "<0={0},1={1}>"));
    pairs.push_back(pair<string, string>("<1={0}>", "<0={0},1={1}>"));
    pairs.push_back(pair<string, string>("<0={0},1={0}>", "<0={0},1={1}>"));
    pairs.push_back(pair<string, string>("<0={0,1}>", "<0={0}>"));

    bool agree[] = {true, true, false, false, true};
    ASSERT_EQ((sizeof(agree) / sizeof(bool)), pairs.size());

    AbstractState a;
    AbstractState b;
    for (int i = 0; i < pairs.size(); ++i) {
        a = AbstractState(pairs[i].first);
        b = AbstractState(pairs[i].second);
        ASSERT_EQ(agree[i], a.agrees_with(b));
        ASSERT_EQ(agree[i], b.agrees_with(a));
    }
}

}
