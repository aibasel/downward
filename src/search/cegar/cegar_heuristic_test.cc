#include <gtest/gtest.h>

#include "abstract_state.h"
#include "abstraction.h"
#include "utils.h"
#include "../operator.h"
#include "../axioms.h"

using namespace std;


namespace cegar_heuristic {
Operator make_op1() {
    // Operator: <0=0, 1=0 --> 1=1>
    vector<string> prevail;
    prevail.push_back("0 0");
    vector<string> pre_post;
    pre_post.push_back("0 1 0 1");
    Operator op = create_op("op1", prevail, pre_post);
    return op;
}

Operator make_op(int cost) {
    vector<string> prevail;
    prevail.push_back("1 1");
    vector<string> pre_post;
    Operator op = create_op("op", prevail, pre_post, cost);
    return op;
}

void init_test() {
    g_axiom_evaluator = new AxiomEvaluator();
    // We have to construct the variable domains.
    // 0 in {0, 1}, 1 in {0, 1, 2}, 2 in {0, 1}.
    g_variable_domain.clear();
    g_variable_domain.push_back(2);
    g_variable_domain.push_back(3);
    g_variable_domain.push_back(2);

    g_variable_name.clear();
    g_variable_name.push_back("var0");
    g_variable_name.push_back("var1");
    g_variable_name.push_back("var2");

    g_initial_state = create_state("0 0 0");

    // Operator: <0=0, 1=0 --> 1=1>
    Operator op1 = make_op1();
    g_operators.clear();
    g_operators.push_back(op1);

    // goal(0) = 1, goal(1) = 1
    g_goal.clear();
    g_goal.push_back(make_pair(0, 1));
    g_goal.push_back(make_pair(1, 1));
}

TEST(CegarTest, str) {
    init_test();

    vector<string> states;
    states.push_back("<0={1}>");
    states.push_back("<>");
    states.push_back("<0={1},1={0,2}>");
    states.push_back("<0={0},1={0,1}>");
    for (int i = 0; i < states.size(); ++i) {
        AbstractState a(states[i]);
        ASSERT_EQ(states[i], a.str());
    }
}

TEST(CegarTest, intersection_empty) {
    set<int> a;
    a.insert(1);
    set<int> b;
    b.insert(1);
    b.insert(2);
    set<int> c;
    c.insert(2);
    set<int> d;
    d.insert(2);
    d.insert(4);
    set<int> e;

    EXPECT_FALSE(intersection_empty(a, b));
    EXPECT_TRUE(intersection_empty(a, c));
    EXPECT_TRUE(intersection_empty(a, d));
    EXPECT_TRUE(intersection_empty(a, e));
    EXPECT_FALSE(intersection_empty(b, c));
    EXPECT_FALSE(intersection_empty(b, d));
    EXPECT_FALSE(intersection_empty(c, d));

    EXPECT_FALSE(intersection_empty(b, a));
    EXPECT_TRUE(intersection_empty(c, a));
    EXPECT_TRUE(intersection_empty(d, a));
    EXPECT_TRUE(intersection_empty(e, a));
    EXPECT_FALSE(intersection_empty(c, b));
    EXPECT_FALSE(intersection_empty(d, b));
    EXPECT_FALSE(intersection_empty(d, c));
}

TEST(CegarTest, regress) {
    init_test();

    // Operator: <0=0, 1=0 --> 1=1>
    Operator op = make_op1();

    vector<pair<string, string> > pairs;
    pairs.push_back(pair<string, string>("<0={0},1={0}>", "<0={0},1={0,1}>"));
    pairs.push_back(pair<string, string>("<0={0},1={0}>", "<>"));
    pairs.push_back(pair<string, string>("<0={0},1={0}>", "<1={1}>"));

    for (int i = 0; i < pairs.size(); ++i) {
        AbstractState a;
        AbstractState b(pairs[i].second);
        b.regress(op, &a);
        EXPECT_EQ(pairs[i].first, a.str());
    }

    vector<string> impossible;
    impossible.push_back("<0={0},1={0}>");
    impossible.push_back("<0={1}>");

    for (int i = 0; i < impossible.size(); ++i) {
        AbstractState a;
        AbstractState b(impossible[i]);
        EXPECT_DEATH(b.regress(op, &a), ".*Assertion .* failed.");
    }
}

TEST(CegarTest, equal) {
    init_test();

    AbstractState a("<0={0},1={0}>");
    AbstractState b("<1={0},0={0}>");
    AbstractState c("<1={0}>");
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a == c);
    ASSERT_FALSE(b == c);
}

TEST(CegarTest, refine_var0) {
    init_test();

    // Operator: <0=0, 1=0 --> 1=1>
    Operator op1 = make_op1();

    AbstractState a;
    a.add_arc(&op1, &a);
    AbstractState *a1 = new AbstractState();
    AbstractState *a2 = new AbstractState();
    a.refine(0, 1, a1, a2);

    string a1s = "<0={0}>";
    string a2s = "<0={1}>";

    // Check refinement hierarchy.
    EXPECT_EQ(a1s, a1->str());
    EXPECT_EQ(a2s, a2->str());
    EXPECT_EQ(0, a.get_refined_var());
    AbstractState *left = a.get_child(0);
    AbstractState *right = a.get_child(1);
    EXPECT_EQ(left, a1);
    EXPECT_EQ(right, a2);
    EXPECT_EQ(a1s, left->str());
    EXPECT_EQ(a1s, a.get_child(0)->str());
    EXPECT_EQ(a2s, a.get_child(1)->str());

    // Check transition system.
    EXPECT_EQ("[(op1,<0={0}>)]", a1->get_next_as_string());
    EXPECT_EQ("[]", a2->get_next_as_string());
}

TEST(CegarTest, refine_var1) {
    init_test();

    // Operator: <0=0, 1=0 --> 1=1>
    Operator op1 = make_op1();

    AbstractState a;
    a.add_arc(&op1, &a);
    AbstractState *a1 = new AbstractState();
    AbstractState *a2 = new AbstractState();
    a.refine(1, 1, a1, a2);

    string a1s = "<1={0,2}>";
    string a2s = "<1={1}>";

    // Check refinement hierarchy.
    EXPECT_EQ(a1s, a1->str());
    EXPECT_EQ(a2s, a2->str());
    EXPECT_EQ(1, a.get_refined_var());
    AbstractState *left = a.get_child(0);
    AbstractState *right = a.get_child(1);
    EXPECT_EQ(left, a1);
    EXPECT_EQ(right, a2);
    EXPECT_EQ(a1s, a.get_child(0)->str());
    EXPECT_EQ(a1s, a.get_child(0)->str());
    EXPECT_EQ(a2s, a.get_child(1)->str());
    EXPECT_EQ(a1s, a.get_child(2)->str());

    // Check transition system.
    EXPECT_EQ("[(op1,<1={1}>)]", a1->get_next_as_string());
    EXPECT_EQ("[]", a2->get_next_as_string());
}

TEST(CegarTest, check_arc) {
    init_test();

    // Operator: <0=0, 1=0 --> 1=1>
    Operator op = make_op1();

    vector<pair<string, string> > pairs;

    pairs.push_back(pair<string, string>("<>", "<0={0},1={1}>"));
    pairs.push_back(pair<string, string>("<0={0}>", "<0={0},1={1}>"));
    pairs.push_back(pair<string, string>("<1={0}>", "<0={0},1={1}>"));
    pairs.push_back(pair<string, string>("<0={0},1={0}>", "<0={0},1={1}>"));
    pairs.push_back(pair<string, string>("<0={0},1={0}>", "<0={1},1={1}>"));
    pairs.push_back(pair<string, string>("<0={0,1}>", "<0={0}>"));
    pairs.push_back(pair<string, string>("<0={1}>", "<>"));

    bool check[] = {
        true, true, true, true, false, true, false
    };
    ASSERT_EQ((sizeof(check) / sizeof(check[0])), pairs.size());

    for (int i = 0; i < pairs.size(); ++i) {
        AbstractState a(pairs[i].first);
        AbstractState b(pairs[i].second);
        ASSERT_EQ(check[i], a.check_and_add_arc(&op, &b));
    }
}

TEST(CegarTest, is_abstraction_of_other) {
    init_test();
    // Check that this variable doesn't appear in the resulting state.
    g_variable_domain.push_back(2);

    typedef pair<string, string> test;
    vector<test> pairs;

    pairs.push_back(test("<>", "<>"));
    pairs.push_back(test("<0={0,1}>", "<0={0,1}>"));

    pairs.push_back(test("<>", "<0={0}>"));
    pairs.push_back(test("<0={0,1}>", "<0={0}>"));

    bool agree[] = {
        true, true, true, true
    };
    bool rev_agree[] = {
        true, true, false, false
    };
    ASSERT_EQ((sizeof(agree) / sizeof(agree[0])), pairs.size());
    ASSERT_EQ((sizeof(rev_agree) / sizeof(agree[0])), pairs.size());

    for (int i = 0; i < pairs.size(); ++i) {
        AbstractState a(pairs[i].first);
        AbstractState b(pairs[i].second);
        EXPECT_EQ(agree[i], a.is_abstraction_of(b));
        EXPECT_EQ(rev_agree[i], b.is_abstraction_of(a));
    }
}

TEST(CegarTest, find_solution_first_state) {
    // Operator: <0=0, 1=0 --> 1=1>
    init_test();

    // -> <>
    Abstraction abs;
    EXPECT_EQ("[(op1,<>)]", abs.init->get_next_as_string());
    // -> 1={0,1} -> 1={2}
    abs.refine(abs.init, 1, 2);

    string a1s = "<1={0,1}>";
    string a2s = "<1={2}>";

    EXPECT_EQ("<>", abs.single->str());
    EXPECT_EQ(a1s, abs.init->str());

    AbstractState *left = abs.single->get_child(0);
    AbstractState *right = abs.single->get_child(2);

    EXPECT_EQ(a1s, left->str());
    EXPECT_EQ(a2s, right->str());

    EXPECT_EQ(a1s, abs.single->get_child(0)->str());
    EXPECT_EQ(a1s, abs.single->get_child(1)->str());
    EXPECT_EQ(a2s, abs.single->get_child(2)->str());

    EXPECT_EQ("[(op1,<1={0,1}>)]", left->get_next_as_string());
    EXPECT_EQ("[]", right->get_next_as_string());

    bool success = abs.find_solution();
    ASSERT_TRUE(success);
    EXPECT_FALSE(abs.init->get_state_out());
    EXPECT_EQ("[<1={0,1}>]", abs.get_solution_string());
}

TEST(CegarTest, find_solution_second_state) {
    // Operator: <0=0, 1=0 --> 1=1>
    init_test();

    // -> <>
    Abstraction abs;
    EXPECT_EQ("[(op1,<>)]", abs.init->get_next_as_string());
    // -> 1={0,2} -> 1={1}
    abs.refine(abs.init, 1, 1);

    string a1s = "<1={0,2}>";
    string a2s = "<1={1}>";

    EXPECT_EQ("<>", abs.single->str());
    EXPECT_EQ(a1s, abs.init->str());

    AbstractState *left = abs.single->get_child(0);
    AbstractState *right = abs.single->get_child(1);

    EXPECT_EQ(a1s, left->str());
    EXPECT_EQ(a2s, right->str());

    EXPECT_FALSE(abs.single->valid());
    EXPECT_TRUE(left->valid());
    EXPECT_TRUE(right->valid());

    EXPECT_EQ(a1s, abs.single->get_child(0)->str());
    EXPECT_EQ(a2s, abs.single->get_child(1)->str());
    EXPECT_EQ(a1s, abs.single->get_child(2)->str());

    EXPECT_EQ("[(op1,<1={1}>)]", left->get_next_as_string());
    EXPECT_EQ("[]", right->get_next_as_string());

    bool success = abs.find_solution();
    ASSERT_TRUE(success);
    EXPECT_EQ("[<1={0,2}>,op1,<1={1}>]", abs.get_solution_string());

    abs.calculate_costs();
    EXPECT_EQ(1, left->get_distance());
    EXPECT_EQ(0, right->get_distance());
}

TEST(CegarTest, find_solution_loop) {
    // Operator: <0=0, 1=0 --> 1=1>
    init_test();
    g_goal.push_back(make_pair(1, 1));

    // -> <>
    Abstraction abs;
    EXPECT_EQ("[(op1,<>)]", abs.init->get_next_as_string());
    // --> 0={0} --> 0={1}  (left state has self-loop).
    abs.refine(abs.init, 0, 1);

    string a1s = "<0={0}>";
    string a2s = "<0={1}>";

    EXPECT_EQ("<>", abs.single->str());
    EXPECT_EQ(a1s, abs.init->str());

    AbstractState *left = abs.single->get_child(0);
    AbstractState *right = abs.single->get_child(1);

    EXPECT_EQ(a1s, left->str());
    EXPECT_EQ(a2s, right->str());

    EXPECT_FALSE(abs.single->valid());
    EXPECT_TRUE(left->valid());
    EXPECT_TRUE(right->valid());

    EXPECT_EQ(a1s, abs.single->get_child(0)->str());
    EXPECT_EQ(a2s, abs.single->get_child(1)->str());

    EXPECT_EQ("[(op1,<0={0}>)]", left->get_next_as_string());
    EXPECT_EQ("[]", right->get_next_as_string());

    bool success = abs.find_solution();
    ASSERT_FALSE(success);

    abs.calculate_costs();
    EXPECT_EQ(INFINITY, left->get_distance());
    EXPECT_EQ(0, right->get_distance());
}

TEST(CegarTest, initialize) {
    // Operator: <0=0, 1=0 --> 1=1>
    init_test();
    g_goal.clear();
    g_goal.push_back(make_pair(1, 1));

    // --> <>
    Abstraction abstraction;
    abstraction.find_solution();
    abstraction.calculate_costs();

    ASSERT_EQ("[<>]", abstraction.get_solution_string());
    EXPECT_EQ(0, abstraction.init->get_distance());

    // --> 1={0,2} --> 1={1}
    bool success = abstraction.check_solution();
    EXPECT_FALSE(success);

    string a1s = "<1={0,2}>";
    string a2s = "<1={1}>";

    EXPECT_EQ(a1s, abstraction.init->str());
    EXPECT_EQ("[(op1,<1={1}>)]", abstraction.init->get_next_as_string());
    AbstractState *right = abstraction.init->get_next()[0].second;
    EXPECT_EQ(a2s, right->str());

    abstraction.find_solution();
    abstraction.calculate_costs();

    ASSERT_EQ("[<1={0,2}>,op1,<1={1}>]", abstraction.get_solution_string());
    EXPECT_EQ(1, abstraction.init->get_distance());
    EXPECT_EQ(0, right->get_distance());

    success = abstraction.check_solution();
    EXPECT_TRUE(success);
}

/*         3
 * h=4 A ----> B h=2
 *      \     /
 *      4\   /5
 *        v v
 *     h=0 C
 */
TEST(CegarTest, astar_search) {
    init_test();
    g_use_metric = true;
    g_goal.clear();
    g_goal.push_back(make_pair(0, 2));

    Operator op1 = make_op(3);
    Operator op2 = make_op(4);
    ASSERT_EQ(4, op2.get_cost());
    Operator op3 = make_op(5);

    Abstraction abs;

    AbstractState a("<0={0}>");
    AbstractState b("<0={1}>");
    AbstractState c("<0={2}>");

    a.add_arc(&op2, &c);
    a.add_arc(&op1, &b);
    b.add_arc(&op3, &c);

    a.set_h(0);
    b.set_h(0);
    c.set_h(0);

    abs.init = &a;
    abs.goal = &c;

    // Run once without heuristic information --> 3 expansions.
    a.set_distance(0);
    b.set_distance(INFINITY);
    c.set_distance(INFINITY);
    abs.queue.push(0, &a);
    bool success = abs.astar_search(true, true);
    ASSERT_TRUE(success);
    // Assert that the solution is a-->b, not a-->b-->c
    AbstractState *found_goal = abs.init->get_state_out();
    EXPECT_EQ("<0={2}>", found_goal->str());
    EXPECT_FALSE(found_goal->get_state_out());
    EXPECT_EQ(3, abs.expansions);
    EXPECT_EQ(4, a.get_h());
    EXPECT_EQ(0, b.get_h());
    EXPECT_EQ(0, c.get_h());

    // Run with heuristic information --> only 2 expansions.
    abs.expansions = 0;
    a.set_h(4);
    b.set_h(2);
    c.set_h(0);
    a.set_distance(0);
    b.set_distance(INFINITY);
    c.set_distance(INFINITY);
    ASSERT_TRUE(abs.queue.empty());
    abs.queue.push(4, &a);
    success = abs.astar_search(true, true);
    ASSERT_TRUE(success);
    // Assert that the solution is a-->b, not a-->b-->c
    found_goal = abs.init->get_state_out();
    EXPECT_EQ("<0={2}>", found_goal->str());
    EXPECT_FALSE(found_goal->get_state_out());
    EXPECT_EQ(2, abs.expansions);
    EXPECT_EQ(4, a.get_h());
    EXPECT_EQ(2, b.get_h());
    EXPECT_EQ(0, c.get_h());
}

/*     3
 * A ----> B
 *  \     /
 *  4\   /5
 *    v v
 *     C
 */
TEST(CegarTest, dijkstra_search) {
    init_test();
    g_use_metric = true;
    g_goal.clear();
    g_goal.push_back(make_pair(0, 2));

    Operator op1 = make_op(3);
    Operator op2 = make_op(4);
    ASSERT_EQ(4, op2.get_cost());
    Operator op3 = make_op(5);

    Abstraction abs;

    AbstractState a("<0={0}>");
    AbstractState b("<0={1}>");
    AbstractState c("<0={2}>");

    a.add_arc(&op2, &c);
    a.add_arc(&op1, &b);
    b.add_arc(&op3, &c);

    abs.init = &a;
    abs.goal = &c;
    a.set_distance(0);
    b.set_distance(INFINITY);
    c.set_distance(INFINITY);
    abs.queue.push(0, &a);
    bool success = abs.astar_search(true, false);
    ASSERT_TRUE(success);
    // Assert that the solution is a-->b, not a-->b-->c
    AbstractState *found_goal = abs.init->get_state_out();
    EXPECT_EQ("<0={2}>", found_goal->str());
    EXPECT_FALSE(found_goal->get_state_out());
    EXPECT_EQ(3, abs.expansions_dijkstra);
}

TEST(CegarTest, partial_ordering1) {
    init_test();
    istringstream iss("begin_CG\n"
                      "2\n"
                      "2 4\n"
                      "1 4\n"
                      "1\n"
                      "2 2\n"
                      "1\n"
                      "1 4\n"
                      "end_CG");
    CausalGraph cg = CausalGraph(iss);
    vector<int> order;
    partial_ordering(cg, &order);
    ASSERT_EQ(3, order.size());
    EXPECT_EQ(0, order[0]);
}

TEST(CegarTest, partial_ordering2) {
    init_test();
    istringstream iss("begin_CG\n"
                      "2\n"
                      "2 4\n"
                      "1 4\n"
                      "1\n"
                      "2 2\n"
                      "0\n"
                      "end_CG");
    CausalGraph cg = CausalGraph(iss);
    vector<int> order;
    partial_ordering(cg, &order);
    ASSERT_EQ(3, order.size());
    EXPECT_EQ(0, order[0]);
    EXPECT_EQ(1, order[1]);
    EXPECT_EQ(2, order[2]);
}
}
