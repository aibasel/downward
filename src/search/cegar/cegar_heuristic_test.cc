#include <gtest/gtest.h>

#include "abstract_state.h"
#include "abstraction.h"
#include "split_tree.h"
#include "utils.h"
#include "values.h"
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

    g_fact_names.clear();
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        vector<string> value_names;
        for (int value = 0; value < g_variable_domain[var]; ++value)
            value_names.push_back(to_string(value));
        g_fact_names.push_back(value_names);
    }

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
        AbstractState abs(states[i]);
        ASSERT_EQ(states[i], abs.str());
    }

    AbstractState a("<0={0},1={0}>");
    AbstractState b("<1={0},0={0}>");
    AbstractState c("<1={0}>");
    ASSERT_TRUE(a.str() == b.str());
    ASSERT_FALSE(a.str() == c.str());
    ASSERT_FALSE(b.str() == c.str());
}

TEST(CegarTest, values) {
    init_test();

    Values v1;

    string buffer;
    vector<string> masks;
    masks.push_back("0000011");
    masks.push_back("0011100");
    masks.push_back("1100000");
    for (int i = 0; i < masks.size(); ++i) {
        to_string(Values::masks[i], buffer);
        ASSERT_EQ(masks[i], buffer);
    }
    EXPECT_EQ("", v1.str());
    v1.set(2, 1);
    EXPECT_EQ("2={1}", v1.str());
    v1.set(1, 1);
    EXPECT_EQ("1={1},2={1}", v1.str());
    v1.add(1, 2);
    EXPECT_EQ("1={1,2},2={1}", v1.str());
    EXPECT_EQ(2, v1.count(0));
    EXPECT_EQ(2, v1.count(1));
    EXPECT_EQ(1, v1.count(2));
}

TEST(CegarTest, domains_intersect) {
    init_test();

    // v1: <0={1},1={0,1}>
    // v2: <0={0,1},1={2},2={1}>
    Values v1, v2;
    v1.set(0, 1);
    v1.remove(1, 2);
    v2.remove(0, 2);
    v2.set(1, 2);
    v2.set(2, 1);

    EXPECT_TRUE(v1.domains_intersect(v2, 0));
    EXPECT_FALSE(v1.domains_intersect(v2, 1));
    EXPECT_TRUE(v1.domains_intersect(v2, 2));
}



TEST(CegarTest, get_rel_conc_states) {
    init_test();

    AbstractState a1("0={0,1},1={0,1}");
    EXPECT_EQ(2/3., a1.get_rel_conc_states());

    AbstractState a2("0={0,1},1={1}");
    EXPECT_EQ(1/3., a2.get_rel_conc_states());

    AbstractState a3("0={0,1},1={0,1,2}");
    EXPECT_EQ(1, a3.get_rel_conc_states());

    AbstractState a4("0={0},1={0},2={1}");
    EXPECT_EQ(1/12., a4.get_rel_conc_states());
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
}

TEST(CegarTest, refine_var0) {
    init_test();

    // Operator: <0=0, 1=0 --> 1=1>
    Operator op1 = make_op1();

    AbstractState a;
    Node node_a(&a);
    a.add_loop(&op1);
    AbstractState *a1 = new AbstractState();
    AbstractState *a2 = new AbstractState();
    vector<int> wanted;
    wanted.push_back(1);
    a.split(0, wanted, a1, a2);

    string a1s = "<0={0}>";
    string a2s = "<0={1}>";

    // Check refinement hierarchy.
    EXPECT_EQ(a1s, a1->str());
    EXPECT_EQ(a2s, a2->str());
    EXPECT_EQ(0, a.get_node()->get_var());
    AbstractState *left = a.get_node()->get_left_child_state();
    AbstractState *right = a.get_node()->get_right_child_state();
    EXPECT_EQ(left, a1);
    EXPECT_EQ(right, a2);
    EXPECT_EQ(a1s, left->str());
    EXPECT_EQ(a1s, a.get_node()->get_left_child_state()->str());
    EXPECT_EQ(a2s, a.get_node()->get_right_child_state()->str());

    // Check transition system.
    ASSERT_EQ(1, a1->get_loops().size());
    EXPECT_EQ("op1", a1->get_loops()[0]->get_name());
    EXPECT_EQ(0, a1->get_next().size());
    EXPECT_EQ(0, a1->get_prev().size());
    EXPECT_EQ(0, a2->get_next().size());
    EXPECT_EQ(0, a2->get_prev().size());
    EXPECT_EQ(0, a2->get_loops().size());
}

TEST(CegarTest, refine_var1) {
    init_test();

    // Operator: <0=0, 1=0 --> 1=1>
    Operator op1 = make_op1();

    AbstractState a;
    Node node_a(&a);
    a.add_loop(&op1);
    AbstractState *a1 = new AbstractState();
    AbstractState *a2 = new AbstractState();
    vector<int> wanted;
    wanted.push_back(1);
    a.split(1, wanted, a1, a2);

    string a1s = "<1={0,2}>";
    string a2s = "<1={1}>";

    // Check refinement hierarchy.
    EXPECT_EQ(a1s, a1->str());
    EXPECT_EQ(a2s, a2->str());
    EXPECT_EQ(1, a.get_node()->get_var());
    AbstractState *left = a.get_node()->get_left_child_state();
    AbstractState *right = a.get_node()->get_right_child_state();
    EXPECT_EQ(left, a1);
    EXPECT_EQ(right, a2);
    EXPECT_EQ(a1s, a.get_node()->get_child(0)->get_abs_state()->str());
    EXPECT_EQ(a1s, a.get_node()->get_child(0)->get_abs_state()->str());
    EXPECT_EQ(a2s, a.get_node()->get_child(1)->get_abs_state()->str());
    EXPECT_EQ(a1s, a.get_node()->get_child(2)->get_abs_state()->str());
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
    Node *root = abs.single->get_node();
    abs.set_max_states_offline(2);
    EXPECT_EQ(0, abs.init->get_next().size());
    EXPECT_EQ(0, abs.init->get_prev().size());
    ASSERT_EQ(1, abs.init->get_loops().size());
    EXPECT_EQ("op1", abs.init->get_loops()[0]->get_name());
    // -> 1={0,1} -> 1={2}

    vector<int> wanted;
    wanted.push_back(2);
    abs.refine(abs.init, 1, wanted);

    string a1s = "<1={0,1}>";
    string a2s = "<1={2}>";

    EXPECT_EQ(a1s, abs.init->str());

    AbstractState *left = root->get_child(0)->get_abs_state();
    AbstractState *test = root->get_child(1)->get_abs_state();
    AbstractState *right = root->get_child(2)->get_abs_state();

    EXPECT_EQ(a1s, left->str());
    EXPECT_EQ(a1s, test->str());
    EXPECT_EQ(a2s, right->str());

    ASSERT_EQ(1, left->get_loops().size());
    EXPECT_EQ("op1", left->get_loops()[0]->get_name());
    EXPECT_EQ(0, left->get_next().size());
    EXPECT_EQ(0, left->get_prev().size());
    EXPECT_EQ(0, right->get_next().size());
    EXPECT_EQ(0, right->get_prev().size());
    EXPECT_EQ(0, right->get_loops().size());

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
    Node *root = abs.single->get_node();
    abs.set_max_states_offline(2);
    // -> 1={0,2} -> 1={1}
    vector<int> wanted;
    wanted.push_back(1);
    abs.refine(abs.init, 1, wanted);

    string a1s = "<1={0,2}>";
    string a2s = "<1={1}>";

    EXPECT_EQ(a1s, abs.init->str());

    AbstractState *left = root->get_child(0)->get_abs_state();
    AbstractState *right = root->get_child(1)->get_abs_state();

    EXPECT_EQ(a1s, left->str());
    EXPECT_EQ(a2s, right->str());

    EXPECT_TRUE(root->is_split());
    EXPECT_FALSE(left->get_node()->is_split());
    EXPECT_FALSE(right->get_node()->is_split());

    EXPECT_EQ(a1s, root->get_child(0)->get_abs_state()->str());
    EXPECT_EQ(a2s, root->get_child(1)->get_abs_state()->str());
    EXPECT_EQ(a1s, root->get_child(2)->get_abs_state()->str());

    bool success = abs.find_solution();
    ASSERT_TRUE(success);
    EXPECT_EQ("[<1={0,2}>,op1,<1={1}>]", abs.get_solution_string());

    abs.update_h_values();
    EXPECT_EQ(1, left->get_distance());
    EXPECT_EQ(0, right->get_distance());
}

TEST(CegarTest, find_solution_loop) {
    // Operator: <0=0, 1=0 --> 1=1>
    init_test();
    g_goal.push_back(make_pair(1, 1));

    // -> <>
    Abstraction abs;
    Node *root = abs.single->get_node();
    abs.set_max_states_offline(2);
    // --> 0={0} --> 0={1}  (left state has self-loop).
    vector<int> wanted;
    wanted.push_back(1);
    abs.refine(abs.init, 0, wanted);

    string a1s = "<0={0}>";
    string a2s = "<0={1}>";

    EXPECT_EQ(a1s, abs.init->str());

    AbstractState *left = root->get_child(0)->get_abs_state();
    AbstractState *right = root->get_child(1)->get_abs_state();

    EXPECT_EQ(a1s, left->str());
    EXPECT_EQ(a2s, right->str());

    EXPECT_TRUE(root->is_split());
    EXPECT_FALSE(left->get_node()->is_split());
    EXPECT_FALSE(right->get_node()->is_split());

    EXPECT_EQ(a1s, root->get_child(0)->get_abs_state()->str());
    EXPECT_EQ(a2s, root->get_child(1)->get_abs_state()->str());

    bool success = abs.find_solution();
    ASSERT_FALSE(success);

    abs.update_h_values();
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
    abstraction.set_max_states_offline(2);
    abstraction.find_solution();
    abstraction.update_h_values();

    ASSERT_EQ("[<>]", abstraction.get_solution_string());
    EXPECT_EQ(0, abstraction.init->get_distance());

    // --> 1={0,2} --> 1={1}
    abstraction.find_solution();
    bool success = abstraction.check_and_break_solution(*g_initial_state, abstraction.init);
    EXPECT_FALSE(success);

    string a1s = "<1={0,2}>";
    string a2s = "<1={1}>";

    EXPECT_EQ(a1s, abstraction.init->str());
    ASSERT_EQ(1, abstraction.init->get_next().size());
    EXPECT_EQ("op1", abstraction.init->get_next()[0].first->get_name());
    EXPECT_EQ(a2s, abstraction.init->get_next()[0].second->str());
    AbstractState *right = abstraction.init->get_next()[0].second;
    EXPECT_EQ(a2s, right->str());

    abstraction.find_solution();
    abstraction.update_h_values();

    ASSERT_EQ("[<1={0,2}>,op1,<1={1}>]", abstraction.get_solution_string());
    EXPECT_EQ(1, abstraction.init->get_distance());
    EXPECT_EQ(0, right->get_distance());

    success = abstraction.check_and_break_solution(*g_initial_state);
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

    AbstractState a("<1={0}>");
    Node node_a(&a);
    AbstractState b("<1={1}>");
    Node node_b(&b);
    AbstractState c("<1={2}>");
    Node node_c(&c);

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
    abs.queue->push(0, &a);
    bool success = abs.astar_search(true, true);
    ASSERT_TRUE(success);
    // Assert that the solution is a-->b, not a-->b-->c
    AbstractState *found_goal = abs.init->get_state_out();
    EXPECT_EQ("<1={2}>", found_goal->str());
    EXPECT_FALSE(found_goal->get_state_out());
    EXPECT_EQ(4, a.get_h());
    EXPECT_EQ(0, b.get_h());
    EXPECT_EQ(0, c.get_h());

    // Run with heuristic information --> only 2 expansions.
    a.set_h(4);
    b.set_h(2);
    c.set_h(0);
    a.set_distance(0);
    b.set_distance(INFINITY);
    c.set_distance(INFINITY);
    ASSERT_TRUE(abs.queue->empty());
    abs.queue->push(4, &a);
    success = abs.astar_search(true, true);
    ASSERT_TRUE(success);
    // Assert that the solution is a-->b, not a-->b-->c
    found_goal = abs.init->get_state_out();
    EXPECT_EQ("<1={2}>", found_goal->str());
    EXPECT_FALSE(found_goal->get_state_out());
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

    AbstractState a("<1={0}>");
    Node node_a(&a);
    AbstractState b("<1={1}>");
    Node node_b(&b);
    AbstractState c("<1={2}>");
    Node node_c(&c);

    a.add_arc(&op2, &c);
    a.add_arc(&op1, &b);
    b.add_arc(&op3, &c);

    abs.init = &a;
    abs.goal = &c;
    a.set_distance(0);
    b.set_distance(INFINITY);
    c.set_distance(INFINITY);
    abs.queue->push(0, &a);
    bool success = abs.astar_search(true, false);
    ASSERT_TRUE(success);
    // Assert that the solution is a-->b, not a-->b-->c
    AbstractState *found_goal = abs.init->get_state_out();
    EXPECT_EQ("<1={2}>", found_goal->str());
    EXPECT_FALSE(found_goal->get_state_out());
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

TEST(CegarTest, split_values) {
    init_test();
    AbstractState a;
    State s1 = *create_state("0 0 0");
    SplitTree t;
    t.set_root(&a);
    EXPECT_EQ("<>", t.get_node(s1)->abs_state->str());

    AbstractState a1("<1={1}>");
    AbstractState a2("<1={0,2}>");
    vector<int> values;
    values.push_back(0);
    values.push_back(2);
    t.root->split(1, values, &a1, &a2);
    EXPECT_EQ(0, t.root->left_child->abs_state);
    EXPECT_EQ(&a2, t.root->right_child->abs_state);
    EXPECT_EQ(&a1, t.root->left_child->left_child->abs_state);
    EXPECT_EQ(&a2, t.root->left_child->right_child->abs_state);
}
}
