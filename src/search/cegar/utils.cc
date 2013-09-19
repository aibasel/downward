#include "utils.h"

#include <cassert>
#include <fstream>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../globals.h"
#include "../operator.h"
#include "../option_parser.h"
#include "../state.h"
#include "../timer.h"
#include "../utilities.h"
#include "../landmarks/h_m_landmarks.h"

using namespace std;

namespace cegar_heuristic {
bool DEBUG = false;
typedef unordered_map<int, unordered_set<int> > Edges;

static std::vector<int> CAUSAL_GRAPH_ORDERING_POS;
static AdditiveHeuristic *additive_heuristic = 0;

Operator create_op(const string desc) {
    istringstream iss("begin_operator\n" + desc + "\nend_operator");
    return Operator(iss, false);
}

Operator create_op(const string name, vector<string> prevail, vector<string> pre_post, int cost) {
    ostringstream oss;
    // Create operator description.
    oss << name << endl << prevail.size() << endl;
    for (int i = 0; i < prevail.size(); ++i)
        oss << prevail[i] << endl;
    oss << pre_post.size() << endl;
    for (int i = 0; i < pre_post.size(); ++i)
        oss << pre_post[i] << endl;
    oss << cost;
    return create_op(oss.str());
}

State *create_state(const string desc) {
    string full_desc = "begin_state\n" + desc + "\nend_state";
    istringstream iss(full_desc);
    return new State(iss);
}

bool is_marked(Operator &op) {
    return op.is_marked();
}

int get_pre(const Operator &op, int var) {
    for (int i = 0; i < op.get_prevail().size(); i++) {
        const Prevail &prevail = op.get_prevail()[i];
        if (prevail.var == var)
            return prevail.prev;
    }
    for (int i = 0; i < op.get_pre_post().size(); i++) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            return pre_post.pre;
    }
    return UNDEFINED;
}

int get_eff(const Operator &op, int var) {
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            return pre_post.post;
    }
    return UNDEFINED;
}

int get_post(const Operator &op, int var) {
    for (int i = 0; i < op.get_prevail().size(); ++i) {
        const Prevail &prevail = op.get_prevail()[i];
        if (prevail.var == var)
            return prevail.prev;
    }
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            return pre_post.post;
    }
    return UNDEFINED;
}

void get_unmet_preconditions(const Operator &op, const State &state, Splits *splits) {
    assert(splits->empty());
    for (int i = 0; i < op.get_prevail().size(); ++i) {
        const Prevail &prevail = op.get_prevail()[i];
        if (state[prevail.var] != prevail.prev) {
            vector<int> wanted;
            wanted.push_back(prevail.prev);
            splits->push_back(make_pair(prevail.var, wanted));
        }
    }
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if ((pre_post.pre != -1) && (state[pre_post.var] != pre_post.pre)) {
            vector<int> wanted;
            wanted.push_back(pre_post.pre);
            splits->push_back(make_pair(pre_post.var, wanted));
        }
    }
    assert(splits->empty() == op.is_applicable(state));
}

void get_unmet_goal_conditions(const State &state, Splits *splits) {
    assert(splits->empty());
    for (int i = 0; i < g_goal.size(); i++) {
        int var = g_goal[i].first;
        int value = g_goal[i].second;
        if (state[var] != value) {
            vector<int> wanted;
            wanted.push_back(value);
            splits->push_back(make_pair(var, wanted));
        }
    }
}

bool goal_var(int var) {
    for (int i = 0; i < g_goal.size(); i++) {
        if (var == g_goal[i].first)
            return true;
    }
    return false;
}

bool test_cegar_goal(const State &state) {
    for (int i = 0; i < g_goal.size(); i++) {
        if (state[g_goal[i].first] != g_goal[i].second) {
            return false;
        }
    }
    return true;
}

bool cheaper(Operator *op1, Operator* op2) {
    return op1->get_cost() < op2->get_cost();
}

struct SortByNumSuccessors {
    Edges successors;

    explicit SortByNumSuccessors(Edges &succ)
        : successors(succ) {
    }

    ~SortByNumSuccessors() {
    }

    bool operator() (int i, int j) {
        // Move nodes with the highest number of successors to the front.
        return (successors[i].size() > successors[j].size());
    }

};

struct SortByNumPredecessors {
    Edges predecessors;

    explicit SortByNumPredecessors(Edges &succ)
        : predecessors(succ) {
    }

    ~SortByNumPredecessors() {
    }

    bool operator() (int i, int j) {
        // Move nodes with the highest number of predecessors to the front.
        // The order will be reversed later.
        return (predecessors[i].size() > predecessors[j].size());
    }

};

void erase_node(int del_node, set<int> &nodes, Edges &predecessors, Edges &successors) {
    nodes.erase(del_node);
    // For all unsorted nodes, delete node from their predecessor and
    // successor lists.
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        unordered_set<int> &pre = predecessors[*it];
        auto pos = find(pre.begin(), pre.end(), del_node);
        if (pos != pre.end())
            pre.erase(pos);
        unordered_set<int> &succ = successors[*it];
        pos = find(succ.begin(), succ.end(), del_node);
        if (pos != succ.end())
            succ.erase(pos);
    }
}

void partial_ordering(Edges &predecessors, Edges &successors, vector<int> *order) {
    assert(order->empty());
    bool debug = true && DEBUG;
    // Set of nodes that still have to be ordered.
    set<int> nodes;
    for (auto it = predecessors.begin(); it != predecessors.end(); ++it) {
        nodes.insert(it->first);
    }
    for (auto it = successors.begin(); it != successors.end(); ++it) {
        nodes.insert(it->first);
    }
    const int num_nodes = nodes.size();
    vector<int> front;
    vector<int> back;
    while (!nodes.empty()) {
        int front_in = INF;
        int front_out = -1;
        vector<int> front_nodes;
        int back_in = -1;
        int back_out = INF;
        vector<int> back_nodes;
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            int node = *it;
            unordered_set<int> &pre = predecessors[node];
            unordered_set<int> &succ = successors[node];
            if (debug) {
                cout << "pre(" << node << "): ";
                for (auto p = pre.begin(); p != pre.end(); ++p)
                    cout << *p << " ";
                cout << "(" << succ.size() << " succ)" << endl;
            }
            // We move all nodes without incoming edges to the front. If there
            // are no such nodes, a node is a better front_node if it has fewer
            // incoming or more outgoing edges.
            if (pre.size() == 0) {
                if (front_in > 0) {
                    front_nodes.clear();
                    front_in = 0;
                }
                front_nodes.push_back(node);
            } else if ((pre.size() < front_in) || ((pre.size() < front_in) && (succ.size() > front_out))) {
                assert(front_nodes.size() <= 1);
                front_nodes.clear();
                front_nodes.push_back(node);
                front_in = pre.size();
                front_out = succ.size();
            }
            // We move all nodes without outgoing edges to the back. If there
            // are no such nodes, a node is a better back_node if it has fewer
            // outgoing edges or more incoming edges.
            if (succ.size() == 0) {
                if (back_out > 0) {
                    back_nodes.clear();
                    back_out = 0;
                }
                back_nodes.push_back(node);
            } else if ((succ.size() < back_out) || ((succ.size() == back_out) && (pre.size() > back_in))) {
                assert(back_nodes.size() <= 1);
                back_nodes.clear();
                back_nodes.push_back(node);
                back_in = pre.size();
                back_out = succ.size();
            }
        }
        assert(!front_nodes.empty());
        assert(!back_nodes.empty());
        sort(front_nodes.begin(), front_nodes.end(), SortByNumSuccessors(successors));
        sort(back_nodes.begin(), back_nodes.end(), SortByNumPredecessors(predecessors));
        for (auto it = front_nodes.begin(); it != front_nodes.end(); ++it) {
            int node = *it;
            front.push_back(node);
            if (debug)
                cout << "Put in front: " << node << endl;
            erase_node(node, nodes, predecessors, successors);
        }
        for (auto it = back_nodes.begin(); it != back_nodes.end(); ++it) {
            int node = *it;
            if (nodes.count(node) == 0)
                // The node may have already been added to the front.
                continue;
            back.push_back(node);
            if (debug)
                cout << "Put in back: " << node << endl;
            erase_node(node, nodes, predecessors, successors);
        }
        if (debug)
            cout << endl;
    }
    reverse(back.begin(), back.end());
    order->insert(order->begin(), front.begin(), front.end());
    order->insert(order->end(), back.begin(), back.end());
    if (order->size() != num_nodes)
        ABORT("Not all nodes have been ordered.");
}

void partial_ordering(const CausalGraph &causal_graph, vector<int> *order) {
    // Create an ordering of the variables in the causal graph. Since the CG is not
    // a DAG, we use the following approximation: Put the vars with few incoming and
    // many outgoing edges in front and vars with few outgoing and many incoming
    // edges in the back.
    // For each variable, maintain sets of predecessor and successor variables
    // that haven't been ordered yet.
    Edges predecessors(g_variable_domain.size());
    Edges successors(g_variable_domain.size());
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        const vector<int> &pre = causal_graph.get_predecessors(var);
        for (int i = 0; i < pre.size(); ++i) {
            predecessors[var].insert(pre[i]);
        }
        const vector<int> &succ = causal_graph.get_successors(var);
        for (int i = 0; i < succ.size(); ++i) {
            successors[var].insert(succ[i]);
        }
    }
    partial_ordering(predecessors, successors, order);
}

int get_pos_in_causal_graph_ordering(int var) {
    if (CAUSAL_GRAPH_ORDERING_POS.empty()) {
        cout << "Start ordering causal graph [t=" << g_timer << "]" << endl;
        vector<int> causal_graph_ordering;
        partial_ordering(*g_causal_graph, &causal_graph_ordering);
        CAUSAL_GRAPH_ORDERING_POS.resize(causal_graph_ordering.size(), UNDEFINED);
        for (int i = 0; i < causal_graph_ordering.size(); ++i) {
            CAUSAL_GRAPH_ORDERING_POS[causal_graph_ordering[i]] = i;
        }
        cout << "Finished ordering causal graph [t=" << g_timer << "]" << endl;
        if (DEBUG) {
            cout << "Causal graph ordering: ";
            for (int pos = 0; pos < causal_graph_ordering.size(); ++pos)
                cout << causal_graph_ordering[pos] << " ";
            cout << endl;
        }
    }
    return CAUSAL_GRAPH_ORDERING_POS[var];
}

void setup_hadd() {
    cout << "Start computing h^add values [t=" << g_timer << "]" << endl;
    Options opts;
    opts.set<int>("cost_type", 0);
    opts.set<int>("memory_padding", 75);
    additive_heuristic = new AdditiveHeuristic(opts);
    additive_heuristic->evaluate(*g_initial_state);
    if (DEBUG) {
        cout << "h^add values for all facts:" << endl;
        for (int var = 0; var < g_variable_domain.size(); ++var) {
            for (int value = 0; value < g_variable_domain[var]; ++value) {
                cout << "  " << var << "=" << value << " " << g_fact_names[var][value]
                     << " cost:" << additive_heuristic->get_cost(var, value) << endl;
            }
        }
        cout << endl;
    }
    cout << "Done computing h^add values [t=" << g_timer << "]" << endl;
}

int get_hadd_estimate_for_initial_state() {
    if (!additive_heuristic)
        setup_hadd();
    return additive_heuristic->get_heuristic();
}

int get_hadd_value(int var, int value) {
    if (!additive_heuristic)
        setup_hadd();
    return additive_heuristic->get_cost(var, value);
}

int get_fact_number(int var, int value) {
    return g_fact_borders[var] + value;
}

Fact get_fact(const LandmarkNode *node) {
    assert(node);
    assert(node->vars.size() == 1);
    int var = node->vars[0];
    int value = node->vals[0];
    return Fact(var, value);
}

Fact get_fact(int fact_number) {
    int var = 0;
    while (g_fact_borders[var] + g_variable_domain[var] <= fact_number)
        ++var;
    int value = fact_number - g_fact_borders[var];
    assert(get_fact_number(var, value) == fact_number);
    return Fact(var, value);
}

void write_landmark_graph() {
    Options opts = Options();
    opts.set<int>("cost_type", 0);
    opts.set<int>("memory_padding", 75);
    opts.set<int>("m", 1);
    opts.set<bool>("reasonable_orders", true);
    opts.set<bool>("only_causal_landmarks", false);
    opts.set<bool>("disjunctive_landmarks", false);
    opts.set<bool>("conjunctive_landmarks", false);
    opts.set<bool>("no_orders", false);
    opts.set<int>("lm_cost_type", 0);
    opts.set<Exploration *>("explor", new Exploration(opts));
    HMLandmarks lm_graph_factory(opts);
    LandmarkGraph *graph = lm_graph_factory.compute_lm_graph();
    if (DEBUG)
        graph->dump();
    const set<LandmarkNode *> &nodes = graph->get_nodes();
    set<LandmarkNode *, LandmarkNodeComparer> nodes2(nodes.begin(), nodes.end());

    ofstream dotfile("landmark-graph.dot");
    if (!dotfile.is_open()) {
        cerr << "dot file for causal graph could not be opened" << endl;
        exit_with(EXIT_CRITICAL_ERROR);
    }
    dotfile << "digraph cg {" << endl;


    for (set<LandmarkNode *>::const_iterator it = nodes2.begin(); it
         != nodes2.end(); it++) {
        LandmarkNode *node_p = *it;
        for (auto parent_it = node_p->parents.begin(); parent_it
             != node_p->parents.end(); ++parent_it) {
            //const edge_type &edge = parent_it->second;
            const LandmarkNode *parent_p = parent_it->first;
            Fact parent_fact = get_fact(parent_p);
            dotfile << "  \"" << g_fact_names[parent_fact.first][parent_fact.second] << "\" -> ";
            Fact node_fact = get_fact(node_p);
            dotfile << "\"" << g_fact_names[node_fact.first][node_fact.second] << "\";" << endl;
            if ((*g_initial_state)[parent_fact.first] == parent_fact.second)
                dotfile << "\"" << g_fact_names[parent_fact.first][parent_fact.second] << "\" [color=green];" << endl;
            if ((*g_initial_state)[node_fact.first] == node_fact.second)
                dotfile << "\"" << g_fact_names[node_fact.first][node_fact.second] << "\" [color=green];" << endl;
        }
    }
    for (int var = 0; var < g_variable_domain.size(); var++) {

    }
    for (int i = 0; i < g_goal.size(); i++) {
        int var = g_goal[i].first;
        int value = g_goal[i].second;
        dotfile << "\"" << g_fact_names[var][value] << "\" [color=red];" << endl;
    }
    dotfile << "}" << endl;
    dotfile.close();
}

void write_causal_graph(const CausalGraph &causal_graph) {
    ofstream dotfile("causal-graph.dot");
    if (!dotfile.is_open()) {
        cerr << "dot file for causal graph could not be opened" << endl;
        exit_with(EXIT_CRITICAL_ERROR);
    }
    dotfile << "digraph cg {" << endl;
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        const vector<int> &successors = causal_graph.get_successors(var);
        for (int i = 0; i < successors.size(); ++i) {
            dotfile << "  " << var << " -> " << successors[i] << ";" << endl;
        }
    }
    for (int i = 0; i < g_goal.size(); i++) {
        int var = g_goal[i].first;
        dotfile << var << " [color=red];" << endl;
    }
    dotfile << "}" << endl;
    dotfile.close();
}

string to_string(int i) {
    stringstream out;
    out << i;
    return out.str();
}

string to_string(const vector<int> &v) {
    string sep = "";
    stringstream out;
    for (int i = 0; i < v.size(); ++i) {
        out << sep << v[i];
        sep = ",";
    }
    return out.str();
}
}
