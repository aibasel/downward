#ifndef CAUSAL_GRAPH_H
#define CAUSAL_GRAPH_H

#include <iosfwd>
#include <vector>
#include <map>
using namespace std;

class Operator;
class Axiom;
class Variable;

class CausalGraph {
    const vector<Variable *> &variables;
    const vector<Operator> &operators;
    const vector<Axiom> &axioms;
    const vector<pair<Variable *, int> > &goals;

    typedef map<Variable *, int> WeightedSuccessors;
    typedef map<Variable *, WeightedSuccessors> WeightedGraph;
    WeightedGraph weighted_graph;
    typedef map<Variable *, int> Predecessors;
    typedef map<Variable *, Predecessors> PredecessorGraph;
    // predecessor_graph is weighted_graph with edges turned around
    PredecessorGraph predecessor_graph;

    typedef vector<vector<Variable *> > Partition;
    typedef vector<Variable *> Ordering;
    Ordering ordering;
    bool acyclic;

    void weigh_graph_from_ops(const vector<Variable *> &variables,
                              const vector<Operator> &operators,
                              const vector<pair<Variable *, int> > &goals);
    void weigh_graph_from_axioms(const vector<Variable *> &variables,
                                 const vector<Axiom> &axioms,
                                 const vector<pair<Variable *, int> > &goals);
    void get_strongly_connected_components(Partition &sccs);
    void calculate_topological_pseudo_sort(const Partition &sccs);
    void calculate_important_vars();
    void dfs(Variable *from);
public:
    CausalGraph(const vector<Variable *> &variables,
                const vector<Operator> &operators,
                const vector<Axiom> &axioms,
                const vector<pair<Variable *, int> > &the_goals);
    ~CausalGraph() {}
    const vector<Variable *> &get_variable_ordering() const;
    bool is_acyclic() const;
    void dump() const;
    void generate_cpp_input(ofstream &outfile,
                            const vector<Variable *> &ordered_vars) const;
};

extern bool g_do_not_prune_variables;

#endif
